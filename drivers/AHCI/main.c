
#include "../PCI/PCI.h"
#include <KNeOS.h>

#define AHCI_BASE     0x400000

#define ATA_DEV_BUSY  0x80
#define ATA_DEV_DRQ   0x08

#define SATA_SIGNATURE_ATA             0x00000101
#define SATA_SIGNATURE_ATAPI           0xEB140101
#define SATA_SIGNATURE_SEMB            0xC33C0101
#define SATA_SIGNATURE_PORT_MULTIPLIER 0x96690101

#define HBA_PORT_IPM_ACTIVE  1
#define HBA_PORT_DET_PRESENT 3

#define HBA_PxCMD_ST  0x0001
#define HBA_PxCMD_FRE 0x0010
#define HBA_PxCMD_FR  0x4000
#define HBA_PxCMD_CR  0x8000
#define HBA_PxIS_TFES 0x40000000


typedef struct _tagHBAPort
{
    QWORD qwCommandListBaseAddress;
    QWORD qwFISBaseAddress;
    DWORD dwInterruptStatus;
    DWORD dwInterruptEnable;
    DWORD dwCommandAndStatus;
    DWORD dwReserved0;
    DWORD dwTaskFileData;
    DWORD dwSignature;
    DWORD dwSATAStatus;
    DWORD dwSATAControl;
    DWORD dwSATAError;
    DWORD dwSATAActive;
    DWORD dwCommandIssue;
    DWORD dwSATANotification;
    DWORD dwFISSwitchControl;
    DWORD arrReserved1[11];
    DWORD arrVendor[4];
} __attribute__((packed)) sHBAPort;

typedef struct _tagHBAMemory
{
    DWORD    nHostCapability;
    DWORD    nGlobalHostControl;
    DWORD    nInterruptStatus;
    DWORD    nPortImplemented;
    DWORD    nVersion;
    DWORD    nCommandCompletionCoalescingControl;
    DWORD    nCommandCompletionCoalescingPorts;
    DWORD    nEnclosureManagementLocation;
    DWORD    nEnclosureManagementControl;
    DWORD    nHostCapabilityExtended;
    DWORD    nBIOSHandoffControlStatus;
    BYTE     arrReserved[116];
    BYTE     arrVendor[96];
    sHBAPort arrPorts[32];
} __attribute__((packed)) sHBAMemory;

typedef struct _tagHBACommandHeader
{
    BYTE nCommandFISLength : 5;
    BOOL bATAPI : 1;
    BOOL bWrite : 1;
    BOOL bPrefetchable : 1;
    BOOL bReset : 1;
    BOOL bBIST : 1;
    BOOL bClearBusyUponOK : 1;
    BYTE nReserved0 : 1;
    BYTE nPortMultiplierPort : 4;
    WORD wPhysicalRegionDescriptorTable;
    volatile DWORD dwPhysicalRegionDescriptorByteCount;
    QWORD qwCommandTableDescriptorBaseAddress;
    DWORD arrReserved1[4];
} __attribute__((packed)) sHBACommandHeader;

sHBAPort *g_arrHBAPorts[32];
sHBAMemory *g_pHBAMemory;
// sList g_lstDevices;

// Start command engine
void StartCommand(sHBAPort *pPort)
{
    // Wait until CR (bit15) is cleared
    while (pPort->dwCommandAndStatus & HBA_PxCMD_CR);

    // Set FRE (bit4) and ST (bit0)
    pPort->dwCommandAndStatus |= HBA_PxCMD_FRE;
    pPort->dwCommandAndStatus |= HBA_PxCMD_ST;
}

// Stop command engine
void StopCommand(sHBAPort *pPort)
{
    // Clear ST (bit0)
    pPort->dwCommandAndStatus &= ~HBA_PxCMD_ST;

    // Clear FRE (bit4)
    pPort->dwCommandAndStatus &= ~HBA_PxCMD_FRE;

    // Wait until FR (bit14), CR (bit15) are cleared
    while (true)
    {
        if (pPort->dwCommandAndStatus & HBA_PxCMD_FR || pPort->dwCommandAndStatus & HBA_PxCMD_CR)
            continue;
        else
            break;
    }
}

void RebasePort(sHBAPort *pPort, int nPort)
{
    StopCommand(pPort);
    pPort->qwCommandListBaseAddress = AHCI_BASE + (nPort << 10);
    // ZeroMemory((PVOID) pPort->qwCommandListBaseAddress, 1024);

    pPort->qwFISBaseAddress = AHCI_BASE + (nPort << 8) + 0x8000;
    // ZeroMemory((PVOID) pPort->qwFISBaseAddress, 256);

    sHBACommandHeader *pCommandHeader = (sHBACommandHeader *) pPort->qwCommandListBaseAddress;
    for (BYTE i = 0; i < 32; i++)
    {
        pCommandHeader[i].wPhysicalRegionDescriptorTable = 8;
        pCommandHeader[i].qwCommandTableDescriptorBaseAddress = AHCI_BASE + (nPort << 13) + (i << 8) + 0xA000;
        // ZeroMemory((PVOID) pCommandHeader[i].qwCommandTableDescriptorBaseAddress, 256);
    }

    StartCommand(pPort);
}

void NeoScanPorts()
{
    DWORD dwPortImplemented = g_pHBAMemory->nPortImplemented;
    for (BYTE i = 0; i < 32; i++, dwPortImplemented >>= 1)
    {
        if (dwPortImplemented & 0x01)
        {
            RebasePort(&g_pHBAMemory->arrPorts[i], i);
            g_arrHBAPorts[i] = &g_pHBAMemory->arrPorts[i];
        }
    }
}

void DriverMain()
{
    PVOID pPCIDriver = KNeoGetDriver("PCI");
    sPCIDeviceDescriptor sDesc;
    ((BOOL (*)(BYTE, BYTE, sPCIDeviceDescriptor *pDesc)) KNeoGetDriverFunction(pPCIDriver, "KNeoGetPCIDevice"))(0x01, 0x06, &sDesc);
    sBaseAddressRegister sBAR5 = ((sBaseAddressRegister (*)(sPCIDeviceDescriptor, BYTE)) KNeoGetDriverFunction(pPCIDriver, "KNeoGetBaseAddressRegister"))(sDesc, 5);
    KNeoMapPagesToIdentity((PVOID) sBAR5.qwAddress, 1, PAGE_WRITEABLE | PAGE_CACHEDISABLE);
    g_pHBAMemory = (sHBAMemory *) sBAR5.qwAddress;    

    KNeoPauseProcess(KNeoGetCurrentPID());
}
