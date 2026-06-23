
#ifndef __RAMFILE_H
#define __RAMFILE_H

#include <common/types.h>

typedef struct _tagRAMFile
{
    struct _tagRAMFile *pParent;
    BOOL  bFolder;
    WCHAR wsFileName[64];
    BYTE  bFileNameLength;
    QWORD qwFileSize;
    PVOID pData;
} sRAMFile;

void SetRoot(sRAMFile *pRoot);
sRAMFile *GetRAMFile(PWCHAR wszPath);

#endif // __RAMFILE_H
