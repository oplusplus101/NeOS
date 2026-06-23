
#include <uefi.h>
#include "fs.h"
#include "string.h"

#define _MIN(a, b) ((a) < (b) ? (a) : (b))
#define _PANIC(...) { printf(__VA_ARGS__); while (1); }
#define _ASSERT(c, ...) if (!(c)) { _PANIC(__VA_ARGS__); }

static sBootSector g_sBootSector;
static efi_block_io_t *g_pIOBlockDevice;
static PBYTE g_pClusterBuffer;
static PDWORD g_arrFAT;
static DWORD g_dwNonDataRegion;
static QWORD g_qwBytesPerCluster;

static sGPTPartitionEntry g_arrPartitionEntries[128];
static sGPTPartitionEntry g_sKernelPartition;

QWORD g_qwBootFSSize = 0;

QWORD GetBootFileSystemSize()
{
    return g_qwBootFSSize;
}

efi_status_t ReadSectors(QWORD qwLBA, QWORD qwLengthInBytes, PVOID pBuffer)
{
    return g_pIOBlockDevice->ReadBlocks(g_pIOBlockDevice, g_pIOBlockDevice->Media->MediaId, qwLBA, qwLengthInBytes, pBuffer);
}

efi_status_t ReadFSSectors(QWORD qwLBA, QWORD qwLengthInBytes, PVOID pBuffer)
{
    return ReadSectors(g_sKernelPartition.qwLBAStart + qwLBA, qwLengthInBytes, pBuffer);
}

BOOL FindFileInFolder(const sBootFileDescriptor *pParent, const PWCHAR wszFileName, sBootFileDescriptor *pResult)
{
    DWORD dwCurrentCluster = pParent->dwFirstCluster;
    _ASSERT(dwCurrentCluster != 0, "Descriptor is either invalid or empty");

    QWORD qwFileNameLength = strlenW(wszFileName);

    while (dwCurrentCluster != 0xFFFFFFFF)
    {
        ReadFSSectors(dwCurrentCluster * (QWORD) g_sBootSector.bSectorsPerCluster, g_qwBytesPerCluster, g_pClusterBuffer);
        
        for (QWORD qwOffset = 0; qwOffset < g_qwBytesPerCluster / sizeof(sBootFileDescriptor) * sizeof(sBootFileDescriptor); qwOffset += sizeof(sBootFileDescriptor))
        {
            sBootFileDescriptor *pDesc = (sBootFileDescriptor *) (g_pClusterBuffer + qwOffset);

            if (pDesc->bType == 0) continue; // The descriptor is empty
            if (pDesc->bFileNameLength != qwFileNameLength) continue; // Optimisation
            if (memcmp(pDesc->wsFileName, wszFileName, qwFileNameLength * sizeof(WCHAR))) continue;

            memcpy(pResult, pDesc, sizeof(sBootFileDescriptor));
            return true;
        }

        dwCurrentCluster = g_arrFAT[dwCurrentCluster - g_dwNonDataRegion];
    }

    return false;
}

// WARNING: wszPath gets written to
sBootFileDescriptor ParsePath(PWCHAR wszPath)
{
    sBootFileDescriptor sRoot = { 0 };
    sRoot.dwFirstCluster = g_dwNonDataRegion;

    for (PWCHAR wszFileName = strtokW(wszPath, L"/\\"); ; wszFileName = strtokW(NULL, L"/\\"))
    {
        if (wszFileName == NULL) // The previous entry was the last
            break;

        _ASSERT(FindFileInFolder(&sRoot, wszFileName, &sRoot), "Could not find '%s'", wszFileName);
    }

    return sRoot;
}

sRAMFile *CollectFolderContents(sBootFileDescriptor *pFSFolder, sRAMFile *pParent)
{
    sRAMFile *pFolder;
    BS->AllocatePool(EfiLoaderData, sizeof(sRAMFile), (PVOID *) &pFolder);
    memset(pFolder, 0, sizeof(sRAMFile));
    pFolder->bFolder         = true;
    pFolder->pParent         = pParent;
    pFolder->bFileNameLength = pFSFolder->bFileNameLength;
    pFolder->qwFileSize      = pFSFolder->qwFileSize;
    memcpy(pFolder->wsFileName, pFSFolder->wsFileName, sizeof(pFSFolder->wsFileName));

    g_qwBootFSSize += sizeof(sRAMFile);

    if (pFSFolder->qwFileSize == 0)
    {
        pFolder->pData = NULL;
        return pFolder;
    }

    BS->AllocatePool(EfiLoaderData, pFolder->qwFileSize * sizeof(sRAMFile **), &pFolder->pData);
    memset(pFolder->pData, 0, pFolder->qwFileSize * sizeof(sRAMFile *));
    g_qwBootFSSize += pFolder->qwFileSize * sizeof(sRAMFile *);
    
    DWORD dwCurrentCluster = pFSFolder->dwFirstCluster;
    DWORD dwFolderOffset = 0;

    PBYTE pLocalClusterBuffer, pLocalFileClusterBuffer;
    BS->AllocatePool(EfiLoaderData, g_qwBytesPerCluster, (PVOID *) &pLocalClusterBuffer);
    BS->AllocatePool(EfiLoaderData, g_qwBytesPerCluster, (PVOID *) &pLocalFileClusterBuffer);

    while (dwCurrentCluster != 0xFFFFFFFF)
    {
        efi_status_t s = ReadFSSectors(dwCurrentCluster * (QWORD) g_sBootSector.bSectorsPerCluster, g_qwBytesPerCluster, pLocalClusterBuffer);
        _ASSERT(!EFI_ERROR(s), "[  ERROR  ] Failed to read folder content sectors [%016X]", s);
        
        for (QWORD qwOffset = 0; qwOffset < g_qwBytesPerCluster; qwOffset += sizeof(sBootFileDescriptor))
        {
            sBootFileDescriptor *pDesc = (sBootFileDescriptor *) (pLocalClusterBuffer + qwOffset);
            if (pDesc->bType == 0) continue; // The descriptor is empty
            if (pDesc->bType == 1) // File
            {
                sRAMFile *pFile;
                BS->AllocatePool(EfiLoaderData, sizeof(sRAMFile), (PVOID *) &pFile);
                g_qwBootFSSize += sizeof(sRAMFile);
                pFile->bFolder         = false;
                pFile->pParent         = pFolder;
                pFile->bFileNameLength = pDesc->bFileNameLength;
                pFile->qwFileSize      = pDesc->qwFileSize;
                memcpy(pFile->wsFileName, pDesc->wsFileName, sizeof(pDesc->wsFileName));
                BS->AllocatePool(EfiLoaderData, pFile->qwFileSize, &pFile->pData);
                g_qwBootFSSize += pFile->qwFileSize;

                QWORD qwToRead = pDesc->qwFileSize;
                QWORD qwOffset = 0;
                DWORD dwCluster = pDesc->dwFirstCluster;

                // Read the file
                while (qwToRead > 0 && dwCluster != 0xFFFFFFFF)
                {
                    QWORD qwReadSize = _MIN(qwToRead, g_qwBytesPerCluster);
                    efi_status_t s = ReadFSSectors(dwCluster * g_sBootSector.bSectorsPerCluster, g_qwBytesPerCluster, pLocalFileClusterBuffer);
                    _ASSERT(!EFI_ERROR(s), "[  ERROR  ] Failed to read file content sectors [%016X]", s);
                    memcpy((PBYTE) pFile->pData + qwOffset, pLocalFileClusterBuffer, qwReadSize);
                    qwToRead -= qwReadSize;
                    qwOffset += qwReadSize;
                    dwCluster = g_arrFAT[dwCluster - g_dwNonDataRegion];
                }

                ((sRAMFile **) pFolder->pData)[dwFolderOffset++] = pFile;
            }
            else if (pDesc->bType == 2) // Folder
            {
                sRAMFile *pSubfolder = CollectFolderContents(pDesc, pFolder);
                ((sRAMFile **) pFolder->pData)[dwFolderOffset++] = pSubfolder;
            }
        }

        dwCurrentCluster = g_arrFAT[dwCurrentCluster - g_dwNonDataRegion];
    }

    BS->FreePool(pLocalClusterBuffer);

    return pFolder;
}

sRAMFile *GetBootFileSystem()
{
    static sRAMFile *pBootFileSystem = NULL;
    if (pBootFileSystem != NULL) return pBootFileSystem;

    g_qwBootFSSize += sizeof(sRAMFile);
    
    sBootFileDescriptor sDesc;
    sDesc.bType = 2;
    sDesc.bFileNameLength = 6;
    sDesc.qwFileSize = 0;
    sDesc.dwFirstCluster = g_dwNonDataRegion;
    sDesc.qwWriteTimeStamp = 0;
    memcpy(sDesc.wsFileName, L"<root>", 14);
    
    // Count the number of entries
    DWORD dwCurrentCluster = g_dwNonDataRegion;
    while (dwCurrentCluster != 0xFFFFFFFF)
    {
        ReadFSSectors(dwCurrentCluster * (QWORD) g_sBootSector.bSectorsPerCluster, g_qwBytesPerCluster, g_pClusterBuffer);
        
        for (QWORD qwOffset = 0; qwOffset < g_qwBytesPerCluster / sizeof(sBootFileDescriptor) * sizeof(sBootFileDescriptor); qwOffset += sizeof(sBootFileDescriptor))
        {
            sBootFileDescriptor *pDesc = (sBootFileDescriptor *) (g_pClusterBuffer + qwOffset);
            if (pDesc->bType == 0) continue; // The descriptor is empty
            sDesc.qwFileSize++;
        }

        dwCurrentCluster = g_arrFAT[dwCurrentCluster - g_dwNonDataRegion];
    }

    pBootFileSystem = CollectFolderContents(&sDesc, NULL);
    return pBootFileSystem;
}

sRAMFile *GetFile(PWCHAR wszPath, sRAMFile *pParent)
{
    sRAMFile *pCurrent = pParent;

    for (PWCHAR wszFileName = strtokW(wszPath, L"/\\"); wszFileName != NULL; wszFileName = strtokW(NULL, L"/\\"))
    {
        if (pCurrent->qwFileSize == 0) continue;
        BYTE bFileNameLength = strlenW(wszFileName);
        // For folders, the qwFileSize field lists the number of files, not the actual file size
        sRAMFile *pSubFile = NULL;
        for (QWORD i = 0; i < pCurrent->qwFileSize; i++)
        {
            sRAMFile *pFile = ((sRAMFile **) pCurrent->pData)[i];
            if (bFileNameLength != pFile->bFileNameLength) continue;
            if (memcmp(pFile->wsFileName, wszFileName, pFile->bFileNameLength * sizeof(WCHAR))) continue;
            pSubFile = pFile;
        }

        if (pSubFile == NULL) return NULL;
        pCurrent = pSubFile;
    }

    return pCurrent;
}

void InitialiseFileSystem()
{
    // GUIDs
    efi_guid_t sLoadedGUID = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    efi_guid_t sBlockIOGUID = EFI_BLOCK_IO_PROTOCOL_GUID;

    
    efi_loaded_image_protocol_t *pImageProtocol;
    BS->HandleProtocol(IM, &sLoadedGUID, (PVOID *) &pImageProtocol);
    _ASSERT(pImageProtocol != NULL, "Could not find the loaded image protocol");


    /* Get block IO from boot device */
    BS->HandleProtocol(
        pImageProtocol->DeviceHandle,
        &sBlockIOGUID,
        (PVOID *) &g_pIOBlockDevice
    );
    
    _ASSERT(g_pIOBlockDevice != NULL, "Could not find the block I/O protocol");

    efi_handle_t *arrHandles;
    uintn_t uNumHandles;

    BS->LocateHandleBuffer(ByProtocol, &sBlockIOGUID, NULL, &uNumHandles, &arrHandles);
    
    g_pIOBlockDevice = NULL;

    for (uintn_t i = 0; i < uNumHandles; i++) {
        efi_block_io_t *pBIO;

        if (EFI_ERROR(BS->HandleProtocol(arrHandles[i], &sBlockIOGUID, (PVOID *) &pBIO)))
            continue;

        // Whole disk, not partition
        if (!pBIO->Media->LogicalPartition) {
            g_pIOBlockDevice = pBIO;
            break;
        }
    }

    _ASSERT(g_pIOBlockDevice != NULL, "Could not find physical disk");
    // TODO: Implement safety checks

    // Load the GPT
    sGPTProtectiveMasterBootRecord mbr;
    sGPTPartitionTableHeader hdr;
    ReadSectors(0, 512, &mbr);
    ReadSectors(1, 512, &hdr);
    ReadSectors(2, 512, g_arrPartitionEntries);

    _ASSERT(mbr.wMagicNumber == 0xAA55, "Invalid MBR magic number: 0x%04X", mbr.wMagicNumber);
    _ASSERT(hdr.qwSignature == 0x5452415020494645, "Invalid GPT signature: 0x%16X", hdr.qwSignature);
    
    BOOL bKernelFound = false;
    for (BYTE i = 0; i < 128; i++)
    {
        if (!memcmp(g_arrPartitionEntries[i].arrPartitionTypeGUID, (BYTE[16]) { 0 }, 16)) continue;
        if (!memcmp(g_arrPartitionEntries[i].arrPartitionTypeGUID, (BYTE[16]) { 0x57, 0x48, 0x4F, 0x20, 0x49, 0x53, 0x20, 0x4C, 0x4F, 0x52, 0x44, 0x3F, 0x20, 0x34, 0x30, 0x36 }, 16))
        {
            memcpy(&g_sKernelPartition, &g_arrPartitionEntries[i], sizeof(sGPTPartitionEntry));
            bKernelFound = true;
            break;
        }
    }

    _ASSERT(bKernelFound, "Boot partition not found");
    
    
    // Read the boot sector
    ReadFSSectors(0, sizeof(sBootSector), &g_sBootSector);
    _ASSERT(g_sBootSector.wBootMagic == 0xAA55, "Invalid MBR magic number 0x%04X", g_sBootSector.wBootMagic);
    _ASSERT(g_sBootSector.qwMagic == 0x38303430534f454e, "Invalid magic number 0x%16X", g_sBootSector.qwMagic);
    _ASSERT(g_sBootSector.wBytesPerSector == g_pIOBlockDevice->Media->BlockSize, "Device and filesystem block sector sizes don't match! Device: %u bytes, Filesystem: %u bytes", g_pIOBlockDevice->Media->BlockSize, g_sBootSector.wBytesPerSector);


    g_qwBytesPerCluster = (QWORD) g_sBootSector.wBytesPerSector * g_sBootSector.bSectorsPerCluster;
    g_dwNonDataRegion = 1 + (g_sBootSector.dwBootFileSystemSize * sizeof(DWORD) + g_qwBytesPerCluster - 1) / g_qwBytesPerCluster;
    
    BS->AllocatePool(EfiLoaderData, g_qwBytesPerCluster, (PVOID *) &g_pClusterBuffer);
    _ASSERT(g_pClusterBuffer != NULL, "Failed to allocate the cluster buffer");

    // Read the FAT
    BS->AllocatePool(EfiLoaderData, g_sBootSector.dwBootFileSystemSize * sizeof(DWORD), (PVOID *) &g_arrFAT);
    _ASSERT(g_arrFAT != NULL, "Failed to allocate the FAT buffer");
    ReadFSSectors(g_sBootSector.bSectorsPerCluster, g_sBootSector.dwBootFileSystemSize * sizeof(DWORD), g_arrFAT);
}
