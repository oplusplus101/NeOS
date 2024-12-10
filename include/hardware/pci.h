
#ifndef __NEOS__PCI_H
#define __NEOS__PCI_H

#include <common/types.h>
#include <hardware/ports.h>

typedef struct
{
    WORD  nBus;
    WORD  nSlot;
    WORD  nFunction;

    WORD  nVendor;
    WORD  nDevice;
    
    BYTE  nRevision;
    BYTE  nInterface;
    BYTE  nSubclass;
    BYTE  nClass;

    BYTE  nInterrupt;

    QWORD nPortBase;
} sPCIDeviceDescriptor;


DWORD PCIRead(BYTE nBus, BYTE nSlot, BYTE nFunction, BYTE nOffset);
void PCIWrite(BYTE nBus, BYTE nSlot, BYTE nFunction, BYTE nOffset, DWORD nValue);

sPCIDeviceDescriptor GetDeviceDescriptor(BYTE nBus, BYTE nSlot, BYTE nFunction);

void ScanPCIDevices();

#endif // __NEOS__PCI_H
