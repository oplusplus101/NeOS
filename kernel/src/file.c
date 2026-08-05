
#include <file.h>
#include <neos.h>
#include <common/memory.h>
#include <common/string.h>
#include <runtime/objects.h>

static sRAMFile *g_pRootFile;
sObjectType *g_pBlockDeviceType;

// For now, this function isn't needed, as ramfiles don't need to be freed. 
static void DestroyBlockDeviceCallback(PVOID pObject)
{
    // sAbstractBlockDevice *pDevice = pObject;
}

void InitialiseBootFiles(sRAMFile *pRoot)
{
    g_pRootFile = pRoot;
    g_pBlockDeviceType = CreateType(MakeString(L"BlockDevice"), sizeof(sAbstractBlockDevice), DestroyBlockDeviceCallback, NULL);
}

sRAMFile *GetRAMFile(PWCHAR wszPath)
{
    sRAMFile *pCurrent = g_pRootFile;

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

QWORD BLKGetRAMFileSize(sAbstractBlockDevice *pSelf)
{
    return ((sRAMFile *) pSelf->pBackend)->qwFileSize;
}

QWORD BLKReadRAMFile(sAbstractBlockDevice *pSelf, QWORD qwStart, QWORD qwLength, PVOID pBuffer)
{
    sRAMFile *pFile = (sRAMFile *) pSelf->pBackend;
    if (qwStart + qwLength >= pFile->qwFileSize) return -1LL;
    memcpy(pBuffer, (PBYTE) pFile->pData + qwStart, qwLength);
}

STATUS CreateBlockDeviceFromRAMFile(sRAMFile *pFile, sAbstractBlockDevice **ppDevice)
{
    STATUS s = CreateObject(g_pBlockDeviceType, NULL, sizeof(sAbstractBlockDevice), (PVOID *) ppDevice);
    if (!_SUCCESSFUL(s))
        return s;
    
    (*ppDevice)->eType    = BLK_RAM_FILE;
    (*ppDevice)->Read     = BLKReadRAMFile;
    (*ppDevice)->GetSize  = BLKGetRAMFileSize;
    (*ppDevice)->pBackend = pFile;
    return NEOS_SUCCESS;
}
