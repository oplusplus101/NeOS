
#include <KNeOS.h>
#include "fat32.h"
sModule *g_pDiskModule;
sFAT32BootSector g_sBootSector;
int g_nSystemDrive;
bool (*KNeoReadDiskSectors)(int nDrive, QWORD qwLBA, QWORD qwSectors, PVOID pBuffer);
int (*KNeoGetSystemDrive)();

void EnableDriver()
{
    g_pDiskModule = KNeoLoadModule("storage");
    KNeoAssert(g_pDiskModule, "Storage module (storage.mod) either not loaded or doesn't exist");
    KNeoReadDiskSectors = KNeoGetModuleFunction(g_pDiskModule, "KNeoReadDiskSectors");
    KNeoGetSystemDrive = KNeoGetModuleFunction(g_pDiskModule, "KNeoGetSystemDrive");
    g_nSystemDrive = KNeoGetSystemDrive();

    // Read the boot sector
    KNeoReadDiskSectors(g_nSystemDrive, 0, 1, &g_sBootSector);
    
    for (;;);
}
