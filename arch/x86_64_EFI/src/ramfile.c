
#include <ramfile.h>
#include <common/memory.h>
#include <common/string.h>

static sRAMFile *g_pRootFile;

void SetRoot(sRAMFile *pRoot)
{
    g_pRootFile = pRoot;
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
