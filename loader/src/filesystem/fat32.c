
#include <filesystem/fat32.h>
#include <hardware/storage/drive.h>
#include <memory/heap.h>
#include <common/screen.h>
#include <common/panic.h>
#include <common/string.h>
#include <common/memory.h>

QWORD g_qwFATRootStart, g_qwFATDataStart,
      g_qwFATStart, g_qwPartitionStart;
WORD g_wBytesPerSector, g_wBytesPerCluster;
BYTE g_nDrive, g_nSectorsPerCluster;

QWORD ClusterToSector(DWORD dwCluster)
{
    return g_qwFATDataStart + ((dwCluster & 0x0FFFFFFF) - 2) * g_nSectorsPerCluster;
}

DWORD GetNextCluster(DWORD dwCurrentCluster)
{
    BYTE arrFATBuffer[512];
    ReadFromDrive(g_nDrive, g_qwFATStart + dwCurrentCluster / 128, 1, arrFATBuffer);
    return ((PDWORD) &arrFATBuffer)[dwCurrentCluster % 128] & 0x0FFFFFFF;
}

// Returns the next cluster; in we've reached the end, the function will return 0
void ReadCluster(DWORD dwCurrentCluster, PVOID pBuffer)
{
    ReadFromDrive(g_nDrive, ClusterToSector(dwCurrentCluster), g_nSectorsPerCluster, pBuffer);
}

BOOL SearchForFileInCluster(PWCHAR wszEntryName, DWORD dwCluster, sFAT32DirectoryEntry *pEntry)
{
    PBYTE pBuffer = KHeapAlloc(g_wBytesPerCluster);
    _ASSERT(pBuffer, L"Could not allocate memory for file search");
    WCHAR wszLongFileNameBuffer[256];
    ZeroMemory(wszLongFileNameBuffer, 256 * sizeof(WCHAR));
    BOOL bLastLFNEntry = false;

    for (; dwCluster != 0x0FFFFFF8 && dwCluster != 0x0FFFFFFE && dwCluster != 0x0FFFFFFF; dwCluster = GetNextCluster(dwCluster))
    {
        ReadCluster(dwCluster, pBuffer);

        for (DWORD i = 0; i < g_wBytesPerCluster / sizeof(sFAT32DirectoryEntry); i++)
        {
            memcpy(pEntry, pBuffer + i * sizeof(sFAT32DirectoryEntry), sizeof(sFAT32DirectoryEntry));
            
            if (*pEntry->sFilename == 0x00) break;
            if (*pEntry->sFilename == 0xE5) continue;

            if (pEntry->nAttributes == FAT32_ATTR_LFN)
            {
                if (bLastLFNEntry)
                {
                    bLastLFNEntry = false;
                    ZeroMemory(wszLongFileNameBuffer, 256 * sizeof(WCHAR));
                }

                sFAT32LongFilenameEntry *pLFNEntry = (sFAT32LongFilenameEntry *) pEntry;
                BYTE nOffset = (pLFNEntry->nSequence & 0x0F) - 1;
                memcpy(&wszLongFileNameBuffer[nOffset * 13], pLFNEntry->sFirstPart, 10);
                memcpy(&wszLongFileNameBuffer[nOffset * 13 + 5], pLFNEntry->sSecondPart, 12);
                memcpy(&wszLongFileNameBuffer[nOffset * 13 + 11], pLFNEntry->sThirdPart, 4);
                continue;
            }

            bLastLFNEntry = true;

            ToUppercaseW(wszLongFileNameBuffer);
            if (!strcmpW(wszLongFileNameBuffer, wszEntryName))
            {
                KHeapFree(pBuffer);
                return true;
            }
        }
    }

    KHeapFree(pBuffer);
    ZeroMemory(pEntry, sizeof(sFAT32DirectoryEntry));
    return false;
}

BOOL GetEntryFromPath(PWCHAR wszPath, sFAT32DirectoryEntry *pEntry)
{
    ToUppercaseW(wszPath); // Will overwrite the caller's string
    PWCHAR wszFileName = strtokW(wszPath, L"/\\");

    for (DWORD i = 0, dwCurrentCluster = 2;
         dwCurrentCluster != 0x0FFFFFF8 && wszFileName != NULL;
         dwCurrentCluster = (pEntry->nClusterHigh << 16) | pEntry->nClusterLow,
         i++, wszFileName = strtokW(NULL, L"/\\"))
    {
        if (!SearchForFileInCluster(wszFileName, dwCurrentCluster, pEntry))
        {
            ZeroMemory(pEntry, sizeof(sFAT32DirectoryEntry));
            return false;
        }
    }

    return true;
}

// Reads the entire file into pBuffer
// Returns the total amount read
void ReadDirectoryEntry(sFAT32DirectoryEntry *pEntry, PVOID pBuffer)
{
    DWORD dwFileSize = pEntry->dwFileSize;
    PVOID pClusterBuffer = KHeapAlloc(g_wBytesPerCluster);
    _ASSERT(pClusterBuffer != NULL, L"Could not allocate memory for directory read");
    
    for (DWORD dwCurrentCluster = (pEntry->nClusterHigh << 16) | pEntry->nClusterLow;
         dwFileSize != 0;
         dwCurrentCluster = GetNextCluster(dwCurrentCluster),
         pBuffer = (PBYTE) pBuffer + g_wBytesPerCluster)
    {
        ReadCluster(dwCurrentCluster, pClusterBuffer);
        if (dwFileSize >= g_wBytesPerCluster)
        {
            dwFileSize -= g_wBytesPerCluster;
            memcpy(pBuffer, pClusterBuffer, g_wBytesPerCluster);
        }
        else
        {
            memcpy(pBuffer, pClusterBuffer, dwFileSize);
            dwFileSize = 0;
        }
    }

    KHeapFree(pClusterBuffer);
}

void LoadFAT32(BYTE nDrive, sGPTPartitionEntry kernelPartition)
{
    sFAT32BootSector sBootSector;
    ReadFromDrive(nDrive, kernelPartition.qwLBAStart, 1, &sBootSector);

    g_nDrive             = nDrive;
    g_wBytesPerSector    = sBootSector.wBytesPerSector;
    g_nSectorsPerCluster = sBootSector.nSectorsPerCluster;
    g_wBytesPerCluster   = g_wBytesPerSector * g_nSectorsPerCluster;
    g_qwPartitionStart   = kernelPartition.qwLBAStart;
    g_qwFATStart         = g_qwPartitionStart + sBootSector.wNumReservedSectors;
    g_qwFATDataStart     = g_qwPartitionStart + sBootSector.wNumReservedSectors + sBootSector.dwSectorsPerFAT * sBootSector.nNumFATs;
    g_qwFATRootStart     = ClusterToSector(sBootSector.dwRootDirectoryCluster);
}
