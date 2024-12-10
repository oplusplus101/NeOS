
#ifndef __NEOS__PCI_H
#define __NEOS__PCI_H

#include <common/types.h>

typedef struct
{
    uint16_t nBus;
    uint16_t nSlot;
    uint16_t nFunction;

    uint16_t nVendor;
    uint16_t nDevice;
    
    uint8_t  nRevision;
    uint8_t  nInterface;
    uint8_t  nSubclass;
    uint8_t  nClass;

    uint8_t  nInterrupt;

    size_t   nPortBase;
} sPCIDeviceDescriptor;


uint32_t PCIRead(uint8_t nBus, uint8_t nSlot, uint8_t nFunction, uint8_t nOffset);
void PCIWrite(uint8_t nBus, uint8_t nSlot, uint8_t nFunction, uint8_t nOffset, uint32_t nValue);

sPCIDeviceDescriptor GetDeviceDescriptor(uint8_t nBus, uint8_t nSlot, uint8_t nFunction);

void ScanPCIDevices();

#endif // __NEOS__PCI_H
