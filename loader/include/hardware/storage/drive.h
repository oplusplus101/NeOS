
#ifndef __HARDWARE__STORAGE__DRIVE_H
#define __HARDWARE__STORAGE__DRIVE_H

#include <common/types.h>

typedef enum
{
    DRIVE_TYPE_AHCI_SATA,
    DRIVE_TYPE_AHCI_SATAPI,
    DRIVE_TYPE_NVME
} eDriveType;

void InitDrives();
void ActivateDrive(BYTE bDrive);
BOOL ReadFromDrive(BYTE bDrive, QWORD qwStart, WORD wCount, PVOID pBuffer);
BOOL WriteToDrive(BYTE bDrive, QWORD qwStart, WORD wCount, PVOID pBuffer);
QWORD GetDriveCapacityInSectors(BYTE nDrive);


#endif // __HARDWARE__STORAGE__DRIVE_H
