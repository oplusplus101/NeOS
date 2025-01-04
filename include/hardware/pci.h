
#ifndef __PCI_H
#define __PCI_H

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

    DWORD nPortBase;
} sPCIDeviceDescriptor;

typedef struct
{
    QWORD nAddress;
    enum {
        PCI_MMAP,
        PCI_IO
    } eType;
    BOOL bPrefetchable;
} sBaseAddressRegister;

sBaseAddressRegister GetBaseAddressRegister(BYTE nBus, BYTE nSlot, BYTE nFunction, BYTE nBAR);
DWORD PCIRead(BYTE nBus, BYTE nSlot, BYTE nFunction, BYTE nOffset);
void PCIWrite(BYTE nBus, BYTE nSlot, BYTE nFunction, BYTE nOffset, DWORD nValue);

sPCIDeviceDescriptor GetDeviceDescriptor(BYTE nBus, BYTE nSlot, BYTE nFunction);

void ScanPCIDevices();

#endif // __PCI_H
