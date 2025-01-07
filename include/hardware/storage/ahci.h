
#ifndef __AHCI_H
#define __AHCI_H

#include <common/types.h>

typedef enum
{
    FIS_TYPE_REG_H2D    = 0x27, // Register FIS - host to device
    FIS_TYPE_REG_D2H    = 0x34, // Register FIS - device to host
    FIS_TYPE_DMA_ACT    = 0x39, // DMA activate FIS - device to host
    FIS_TYPE_DMA_SETUP  = 0x41, // DMA setup FIS - bidirectional
    FIS_TYPE_DATA       = 0x46, // Data FIS - bidirectional
    FIS_TYPE_BIST       = 0x58, // BIST activate FIS - bidirectional
    FIS_TYPE_PIO_SETUP  = 0x5F, // PIO setup FIS - device to host
    FIS_TYPE_DEV_BITS   = 0xA1  // Set device bits FIS - device to host
} eFISType;

typedef enum
{
    AHCI_DEVICE_TYPE_NULL,
    AHCI_DEVICE_TYPE_SATA,
    AHCI_DEVICE_TYPE_SEMB,
    AHCI_DEVICE_TYPE_PORT_MULTIPLIER,
    AHCI_DEVICE_TYPE_SATAPI
} eAHCIDeviceType;

typedef enum
{
    ATA_CMD_READ_DMA_EXT = 0x25,
    ATA_CMD_WRITE_DMA_EXT = 0x35,
} eATACommand;

#define AHCI_BASE     0x400000

#define ATA_DEV_BUSY 0x80
#define ATA_DEV_DRQ 0x08

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

typedef struct tagFISRegisterHostToDevice
{
    BYTE  nFISType;
    BYTE  nPortMultiplier : 4;
    BYTE  nReserved0 : 3;
    BOOL  bCommand : 1;
    BYTE  nCommand;
    BYTE  nFeatureLow;
    DWORD nLBALow : 24;
    BYTE  nDevice;
    DWORD nLBAHigh : 24;
    BYTE  nFeatureHigh;
    WORD  nCount;
    BYTE  nIsochronousCommandCompletion;
    BYTE  nControl;
    BYTE  arrReserved1[4];
} sFISRegisterHostToDevice;

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

typedef struct tagHBACommandHeader
{
    BYTE nCommandFISLength : 5;
    BOOL bATAPI : 1;
    BOOL bWrite : 1;
    BOOL bPrefetchable : 1;
    BOOL bReset:1;
    BOOL bBIST:1;
    BOOL bClearBusyUponOK : 1;
    BYTE nReserved0 : 1;
    BYTE nPortMultiplierPort : 4;
    WORD wPhysicalRegionDescriptorTable;
    volatile DWORD dwPhysicalRegionDescriptorByteCount;
    QWORD qwCommandTableDescriptorBaseAddress;
    DWORD arrReserved1[4];
} __attribute__((packed)) sHBACommandHeader;

typedef struct tagHBA_PRDT_ENTRY
{
    QWORD qwDataBaseAddress;      // Data base address
    DWORD dwReserved0;     // Reserved
    DWORD nByteCount : 22;       // Byte count, 4M max
    WORD  nReserved1 : 9;       // Reserved
    BOOL  bInterruptOnCompletion : 1;      // Interrupt on completion
} HBA_PRDT_ENTRY;

typedef struct tagHBACommandTable
{
    BYTE arrCommandFIS[64];
    BYTE arrATAPICommand[16];
    BYTE arrReserved[48];
    HBA_PRDT_ENTRY arrPhysicalRegionDescriptorTableEntries[65535];
} sHBACommandTable;

sHBAPort *GetHBAPort(BYTE nPort);

BOOL AHCIRead(sHBAPort *pPort, QWORD qwStart, WORD wCount, WORD *pBuffer);
BOOL AHCIWrite(sHBAPort *pPort, QWORD qwStart, WORD wCount, WORD *pBuffer);

void SetupAHCI(BYTE nBus, BYTE nSlot, BYTE nFunction);

#endif // __AHCI_H
