
#ifndef __HARDWARE__PCI_H
#define __HARDWARE__PCI_H

#include <common/types.h>
#include <hardware/ports.h>

typedef struct
{
    WORD   nBus;
    WORD   nSlot;
    WORD   nFunction;

    WORD   nVendor;
    WORD   nDevice;
    
    BYTE   nRevision;
    BYTE   nInterface;
    BYTE   nSubclass;
    BYTE   nClass;

    BYTE   nInterrupt;

    DWORD dwPortBase;
} sPCIDeviceDescriptor;

typedef struct
{
    QWORD qwAddress;
    enum
    {
        PCI_MMAP,
        PCI_IO
    } eType;
    BOOL bPrefetchable;
} sBaseAddressRegister;

sBaseAddressRegister GetBaseAddressRegister(BYTE nBus, BYTE nSlot, BYTE nFunction, BYTE nBAR);
DWORD PCIRead(BYTE nBus, BYTE nSlot, BYTE nFunction, BYTE nOffset);
void PCIWrite(BYTE nBus, BYTE nSlot, BYTE nFunction, BYTE nOffset, DWORD dwValue);

sPCIDeviceDescriptor GetDeviceDescriptor(BYTE nBus, BYTE nSlot, BYTE nFunction);

void ScanPCIDevices();

#endif // __HARDWARE__PCI_H
