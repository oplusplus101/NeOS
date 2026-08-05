
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

typedef struct _tagAbstractBlockDevice
{
    QWORD (*GetSize)(struct _tagAbstractBlockDevice *pSelf);
    QWORD (*Read)(struct _tagAbstractBlockDevice *pSelf, QWORD qwStart, QWORD qwLength, PVOID pBuffer);
    enum
    {
        BLK_RAM_FILE
    } eType;
    PVOID pBackend;
} sAbstractBlockDevice;

void InitialiseBootFiles(sRAMFile *pRoot);
sRAMFile *GetRAMFile(PWCHAR wszPath);
STATUS CreateBlockDeviceFromRAMFile(sRAMFile *pFile, sAbstractBlockDevice **ppDevice);

#endif // __RAMFILE_H
