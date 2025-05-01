
#include <KNeOS.h>

typedef struct
{ 
    CHAR szDriverName[257];
    QWORD qwSize;
    BOOL (*ReadSector)(QWORD, QWORD, PVOID);
    BOOL (*WriteSector)(QWORD, QWORD, PVOID);
} sDrive;

// There will be 26 indexable drives
DWORD g_dwActiveDrives;
sDrive g_arrDrives[32];

BYTE DriveToIndex(CHAR c)
{
    if (c >= 'a' && c <= 'z')
        c -= 'a' - 'A';
    return c - 'A';
}

FUNC_EXPORT BOOL NeoDoesDriveExist(CHAR c)
{
    return (g_dwActiveDrives & (1 << DriveToIndex(c))) != 0;
}

FUNC_EXPORT BOOL KNeoRegisterStorageDriver()
{

}

FUNC_EXPORT BOOL KNeoReadSectors(CHAR cDrive, QWORD qwStart, QWORD qwSize, PVOID pBuffer)
{
    if (!NeoDoesDriveExist(cDrive)) return false;

    return g_arrDrives[DriveToIndex(cDrive)].ReadSector(qwStart, qwSize, pBuffer);
}


FUNC_EXPORT BOOL KNeoWriteSectors(CHAR cDrive, QWORD qwStart, QWORD qwSize, PVOID pBuffer)
{
    if (!NeoDoesDriveExist(cDrive)) return false;

    return g_arrDrives[DriveToIndex(cDrive)].WriteSector(qwStart, qwSize, pBuffer);
}

FUNC_EXPORT BOOL NeoReScanDrives()
{

}

void ModuleEntry()
{
    
}
