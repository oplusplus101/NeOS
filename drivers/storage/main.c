
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

BOOL NeoDoesDriveExist(CHAR c)
{
    return (g_dwActiveDrives & (1 << DriveToIndex(c))) != 0;
}

BOOL KNeoRegisterStorageDriver()
{
    return false;
}

BOOL KNeoReadSectors(CHAR cDrive, QWORD qwStart, QWORD qwSize, PVOID pBuffer)
{
    if (!NeoDoesDriveExist(cDrive)) return false;

    return g_arrDrives[DriveToIndex(cDrive)].ReadSector(qwStart, qwSize, pBuffer);
}


BOOL KNeoWriteSectors(CHAR cDrive, QWORD qwStart, QWORD qwSize, PVOID pBuffer)
{
    if (!NeoDoesDriveExist(cDrive)) return false;

    return g_arrDrives[DriveToIndex(cDrive)].WriteSector(qwStart, qwSize, pBuffer);
}

BOOL NeoReScanDrives()
{
    return false;
}

void DriverMain()
{
    
}
