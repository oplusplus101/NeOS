
#include <hardware/storage/drive.h>
#include <hardware/storage/ahci.h>
#include <common/panic.h>
#include <common/memory.h>
#include <memory/paging.h>

DWORD g_nActiveDrives;
PVOID g_pReadBuffer;

void InitDrives()
{
    g_pReadBuffer = AllocatePage();
}

void ActivateDrive(BYTE nDrive)
{
    g_nActiveDrives |= 1 << nDrive;
}

BOOL CheckDrive(BYTE nDrive)
{
    return (g_nActiveDrives >> nDrive) & 0x01;
}

// Reads sectors up to 65536 sectors.
BOOL ReadFromDrive(BYTE nDrive, QWORD qwStart, WORD wCount, PVOID pBuffer)
{
    // _ASSERT((QWORD) GetPhysicalAddress(pBuffer) != _ALIGN_TO_PAGE((QWORD) pBuffer), "Tried to read into a non-identity mapped page");
    _ASSERT(wCount > 0, "Tried to read 0 sectors from drive #%d", nDrive);
    if (!CheckDrive(nDrive)) _KERNEL_PANIC("Tried to read from a drive that doesn't exist.\nDrive: #%d", nDrive);
    switch (nDrive)
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

BOOL WriteToDrive(BYTE nDrive, QWORD qwStart, WORD wCount, PVOID pBuffer)
{
    if (!CheckDrive(nDrive)) _KERNEL_PANIC("Tried to read from a drive that doesn't exist.\nDrive: #%d", nDrive);
    return false;
}
