
#include <hardware/storage/ahci.h>
#include <hardware/storage/drive.h>
#include <memory/paging.h>
#include <common/screen.h>
#include <common/panic.h>
#include <common/memory.h>
#include <hardware/pci.h>

sHBAPort *g_arrDevicePorts[32];
DWORD g_nActiveDevices;

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
    memset((PVOID) pPort->qwCommandListBaseAddress, 0, 1024);

    pPort->qwFISBaseAddress = AHCI_BASE + (nPort << 8) + 0x8000;
    memset((PVOID) pPort->qwFISBaseAddress, 0, 256);

    sHBACommandHeader *pCommandHeader = (sHBACommandHeader *) pPort->qwCommandListBaseAddress;
    for (BYTE i = 0; i < 32; i++)
    {
        pCommandHeader[i].wPhysicalRegionDescriptorTable = 8;
        pCommandHeader[i].qwCommandTableDescriptorBaseAddress = AHCI_BASE + (nPort << 13) + (i << 8) + 0xA000;
        memset((PVOID) pCommandHeader[i].qwCommandTableDescriptorBaseAddress, 0, 256);
    }

    StartCommand(pPort);
}

eAHCIDeviceType GetDeviceType(sHBAPort *pPort)
{
    if ((pPort->dwSATAStatus & 0x0F) != HBA_PORT_DET_PRESENT)
        return AHCI_DEVICE_TYPE_NULL;
    if (((pPort->dwSATAStatus >> 8) & 0x0F) != HBA_PORT_IPM_ACTIVE)
        return AHCI_DEVICE_TYPE_NULL;

    switch (pPort->dwSignature)
    {
    case SATA_SIGNATURE_ATAPI:
        return AHCI_DEVICE_TYPE_SATAPI;
    case SATA_SIGNATURE_SEMB:
        return AHCI_DEVICE_TYPE_SEMB;
    case SATA_SIGNATURE_PORT_MULTIPLIER:
        return AHCI_DEVICE_TYPE_PORT_MULTIPLIER;
    default:
        return AHCI_DEVICE_TYPE_SATA;
    }
}

void FindPorts(sHBAMemory *pMemory)
{
    DWORD dwPortImplemented = pMemory->nPortImplemented;
    for (BYTE i = 0; i < 32; i++, dwPortImplemented >>= 1)
    {
        if (dwPortImplemented & 0x01)
        {
            BYTE nDeviceType = GetDeviceType(&pMemory->arrPorts[i]);
            RebasePort(&pMemory->arrPorts[i], i);
            g_arrDevicePorts[nDeviceType] = &pMemory->arrPorts[i];
            g_nActiveDevices |= 1 << nDeviceType;
            switch (nDeviceType)
            {
            case AHCI_DEVICE_TYPE_SATA:
                ActivateDrive(DRIVE_TYPE_AHCI_SATA);
                break;
            case AHCI_DEVICE_TYPE_SATAPI:
                ActivateDrive(DRIVE_TYPE_AHCI_SATA);
                break;
            }
        }
    }
}

// Find a free command list slot
int FindCommandSlot(sHBAPort *pPort)
{
    DWORD dwSlots = (pPort->dwSATAActive | pPort->dwCommandIssue);
    for (int i = 0; i < 32; i++)
    {
        if ((dwSlots & 0x01) == 0) return i;
        dwSlots >>= 1;
    }
    PrintString("Cannot find free command list entry\n");
    return -1;
}

BOOL AHCIRead(sHBAPort *pPort, QWORD nStart, WORD wCount, WORD *pBuffer)
{
    if (pPort == NULL) return false;
    pPort->dwInterruptStatus = (DWORD) -1;
    int nSpin = 0;
    int nSlot = FindCommandSlot(pPort);
    if (nSlot == -1)
        return false;

    sHBACommandHeader *pCommandHeader = (sHBACommandHeader*) pPort->qwCommandListBaseAddress;
    pCommandHeader += nSlot;
    pCommandHeader->nCommandFISLength = sizeof(sFISRegisterHostToDevice) / sizeof(DWORD);
    pCommandHeader->bWrite = false;
    pCommandHeader->wPhysicalRegionDescriptorTable = (WORD) ((wCount - 1) >> 4) + 1;

    sHBACommandTable *pCommandTable = (sHBACommandTable *)(pCommandHeader->qwCommandTableDescriptorBaseAddress);
    memset(pCommandTable, 0, sizeof(sHBACommandTable) + (pCommandHeader->wPhysicalRegionDescriptorTable - 1) * sizeof(HBA_PRDT_ENTRY));

    WORD i;
    for (i = 0; i < pCommandHeader->wPhysicalRegionDescriptorTable - 1; i++)
    {
        pCommandTable->arrPhysicalRegionDescriptorTableEntries[i].qwDataBaseAddress = (QWORD) pBuffer;
        pCommandTable->arrPhysicalRegionDescriptorTableEntries[i].nByteCount = 8191;
        pCommandTable->arrPhysicalRegionDescriptorTableEntries[i].bInterruptOnCompletion = true;
        pBuffer += 4096;
        wCount -= 16;
    }

    pCommandTable->arrPhysicalRegionDescriptorTableEntries[i].qwDataBaseAddress = (QWORD) pBuffer;
    pCommandTable->arrPhysicalRegionDescriptorTableEntries[i].nByteCount = (wCount << 9) - 1;
    pCommandTable->arrPhysicalRegionDescriptorTableEntries[i].bInterruptOnCompletion = true;

    // Setup command
    sFISRegisterHostToDevice *pCommandFIS = (sFISRegisterHostToDevice *) (&pCommandTable->arrCommandFIS);

    pCommandFIS->nFISType = FIS_TYPE_REG_H2D;
    pCommandFIS->bCommand = true;
    pCommandFIS->nCommand = ATA_CMD_READ_DMA_EXT;

    pCommandFIS->nLBALow = nStart & 0xFFFFFF;
    pCommandFIS->nLBAHigh = nStart >> 24;
    pCommandFIS->nDevice = 64;

    pCommandFIS->wCount = wCount;

    while ((pPort->dwTaskFileData & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && nSpin < 1000000) nSpin++;

    if (nSpin == 1000000)
    {
        PrintString("Port is hung\n");
        return false;
    }

    pPort->dwCommandIssue = 1 << nSlot;

    // Wait for completion
    while (1)
    {
        if ((pPort->dwCommandIssue & (1 << nSlot)) == 0) 
            break;

        if (pPort->dwInterruptStatus & HBA_PxIS_TFES) // Task file error
        {
            PrintString("Read disk error\n");
            return false;
        }
    }

    // Check again
    if (pPort->dwInterruptStatus & HBA_PxIS_TFES)
    {
        PrintString("Read disk error\n");
        return false;
    }

    return true;
}

BOOL AHCIWrite(sHBAPort *pPort, QWORD qwStart, WORD wCount, WORD *pBuffer)
{
    return false;
}

sHBAPort *GetHBAPort(BYTE nPort)
{
    if (((g_nActiveDevices >> nPort) & 0x01) != 0x01) return NULL;
    return g_arrDevicePorts[nPort];
}

void SetupAHCI(BYTE nBus, BYTE nSlot, BYTE nFunction)
{
    sBaseAddressRegister bar5 = GetBaseAddressRegister(nBus, nSlot, nFunction, 5);
    MapPage((PVOID) bar5.qwAddress, (PVOID) bar5.qwAddress, PF_CACHEDISABLE | PF_WRITEABLE);

    g_nActiveDevices = 0;

    sHBAMemory *pHBAMemory = (sHBAMemory *) bar5.qwAddress;
    FindPorts(pHBAMemory);
}
