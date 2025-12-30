
#include <hardware/storage/drive.h>
#include <hardware/storage/ahci.h>

#include <common/memory.h>
#include <common/panic.h>
#include <common/log.h>

#include <memory/paging.h>

DWORD g_dwActiveDrives;
PVOID g_pReadBuffer;

void InitDrives()
{
    g_pReadBuffer = AllocatePage();
    g_dwActiveDrives = 0; // No drives by default
}

void ActivateDrive(BYTE bDrive)
{
    g_dwActiveDrives |= 1 << bDrive;
}

BOOL CheckDrive(BYTE bDrive)
{
    return (g_dwActiveDrives >> bDrive) & 0x01;
}


// Reads sectors up to 65536 sectors.
BOOL ReadFromDrive(BYTE bDrive, QWORD qwStart, WORD wCount, PVOID pBuffer)
{
    _ASSERT(wCount > 0, L"Tried to read 0 sectors from drive #%d", bDrive);
    _ASSERT(CheckDrive(bDrive), L"Tried to read from a drive that doesn't exist.\nDrive: #%d", bDrive);

    switch (bDrive)
    {
    case DRIVE_TYPE_AHCI_SATA:
        // This code is necessary because DMA cannot read into non-identity-mapped memory.
        if (wCount * 512 <= PAGE_SIZE)
        {
            if (!AHCIRead(GetHBAPort(AHCI_DEVICE_TYPE_SATA), qwStart, wCount, g_pReadBuffer))
                return false;
            memcpy(pBuffer, g_pReadBuffer, wCount * 512);
            return true;
        }
        else
        {
            for (DWORD i = 0; i < wCount * 512 / PAGE_SIZE; i++)
            {
                if (!AHCIRead(GetHBAPort(AHCI_DEVICE_TYPE_SATA), qwStart, wCount, g_pReadBuffer))
                    return false;
                memcpy((PVOID) ((QWORD) pBuffer + i * PAGE_SIZE), g_pReadBuffer, PAGE_SIZE);
            }
            return true;
        }
    case DRIVE_TYPE_AHCI_SATAPI: return AHCIRead(GetHBAPort(AHCI_DEVICE_TYPE_SATAPI), qwStart, wCount, pBuffer);
    }
    return false;
}

BOOL WriteToDrive(BYTE bDrive, QWORD qwStart, WORD wCount, PVOID pBuffer)
{
    if (!CheckDrive(bDrive)) _KERNEL_PANIC(L"Tried to read from a drive that doesn't exist.\bDrive: #%d", bDrive);
    return false;
}
