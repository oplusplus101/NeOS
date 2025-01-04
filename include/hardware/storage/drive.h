
#ifndef __DRIVE_H
#define __DRIVE_H

#include <common/types.h>

typedef enum
{
    DRIVE_TYPE_AHCI_SATA,
    DRIVE_TYPE_AHCI_SATAPI,
    DRIVE_TYPE_NVME
} eDriveType;

void InitDrive();
void ActivateDrive(BYTE nDrive);
BOOL ReadFromDrive(BYTE nDrive, QWORD nStart, WORD nSectors, PVOID pBuffer);
BOOL WriteToDrive(BYTE nDrive, QWORD nStart, WORD nSectors, PVOID pBuffer);

#endif // __DRIVE_H
