
#include <KNeOS.h>
#include "fat32.h"
sDriver *g_pDiskDriver;
sFAT32BootSector g_sBootSector;
int g_nSystemDrive;
bool (*KNeoReadDiskSectors)(int nDrive, QWORD qwLBA, QWORD qwSectors, PVOID pBuffer);
int (*KNeoGetSystemDrive)();

void DriverMain()
{
    g_pDiskDriver = KNeoGetDriver("storage");
    _ASSERT(g_pDiskDriver, "Storage Driver (storage.mod) either not loaded or doesn't exist");
    KNeoReadDiskSectors = KNeoGetDriverFunction(g_pDiskDriver, "KNeoReadDiskSectors");
    KNeoGetSystemDrive = KNeoGetDriverFunction(g_pDiskDriver, "KNeoGetSystemDrive");
    g_nSystemDrive = KNeoGetSystemDrive();

    // Read the boot sector
    KNeoReadDiskSectors(g_nSystemDrive, 0, 1, &g_sBootSector);
    
    for (;;);
}
