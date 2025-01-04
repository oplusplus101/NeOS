
#include <hardware/storage/drive.h>
#include <hardware/storage/ahci.h>
#include <common/panic.h>

DWORD g_nActiveDrives;

void InitDrive()
{

}

void ActivateDrive(BYTE nDrive)
{
    g_nActiveDrives |= 1 << nDrive;
}

BOOL CheckDrive(BYTE nDrive)
{
    return (g_nActiveDrives >> nDrive) & 0x01;
}

BOOL ReadFromDrive(BYTE nDrive, QWORD nStart, WORD nSectors, PVOID pBuffer)
{
    if (!CheckDrive(nDrive)) _KernelPanic("Tried to read from a drive that doesn't exist.\nDrive: #%d", nDrive);
    switch (nDrive)
    {
    case DRIVE_TYPE_AHCI_SATA:   return AHCIRead(GetHBAPort(AHCI_DEVICE_TYPE_SATA), nStart, nSectors, pBuffer);
    case DRIVE_TYPE_AHCI_SATAPI: return AHCIRead(GetHBAPort(AHCI_DEVICE_TYPE_SATAPI), nStart, nSectors, pBuffer);
    }
    return false;
}

BOOL WriteToDrive(BYTE nDrive, QWORD nStart, WORD nSectors, PVOID pBuffer)
{
    if (!CheckDrive(nDrive)) _KernelPanic("Tried to read from a drive that doesn't exist.\nDrive: #%d", nDrive);
    return false;
}
