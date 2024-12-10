
#include <hardware/pci.h>
#include <hardware/ports.h>
#include <common/screen.h>

DWORD PCIRead(BYTE nBus, BYTE nSlot, BYTE nFunction, BYTE nOffset)
{
  
    DWORD nAddress = ((DWORD) nBus << 16) | ((DWORD) nSlot << 11) |
                        ((DWORD) nFunction << 8) | ((DWORD) nOffset & 0xFC) | 0x80000000;

    outl(0xCF8, nAddress);
    return inl(0xCFC) >> ((nOffset & 3) << 3);
}

void PCIWrite(BYTE nBus, BYTE nSlot, BYTE nFunction, BYTE nOffset, DWORD nValue)
{
  
    DWORD nAddress = ((DWORD) nBus << 16) | ((DWORD) nSlot << 11) |
                        ((DWORD) nFunction << 8) | ((DWORD) nOffset & 0xFC) | 0x80000000;

    outl(0xCF8, nAddress);
    outl(0xCFC, nValue);
}

sPCIDeviceDescriptor GetDeviceDescriptor(BYTE nBus, BYTE nSlot, BYTE nFunction)
{
    sPCIDeviceDescriptor desc;

    desc.nBus       = nBus;
    desc.nSlot      = nSlot;
    desc.nBus       = nBus;

    desc.nVendor    = PCIRead(nBus, nSlot, nFunction, 0);
    desc.nDevice    = PCIRead(nBus, nSlot, nFunction, 2);

    desc.nRevision  = PCIRead(nBus, nSlot, nFunction, 8);
    desc.nInterface = PCIRead(nBus, nSlot, nFunction, 9);
    desc.nSubclass  = PCIRead(nBus, nSlot, nFunction, 10);
    desc.nClass     = PCIRead(nBus, nSlot, nFunction, 11);

    desc.nInterrupt = PCIRead(nBus, nSlot, nFunction, 60);

    return desc;
}

void ScanPCIDevices()
{
    for (int nBus = 0; nBus < 8; nBus++)
        for (int nSlot = 0; nSlot < 32; nSlot++)
        {
            int nFunctions = PCIRead(nBus, nSlot, 0, 14) ? 8 : 1;
            for (int nFunction = 0; nFunction < nFunctions; nFunction++)
            {
                sPCIDeviceDescriptor desc = GetDeviceDescriptor(nBus, nSlot, nFunction);
                if (desc.nVendor == 0 || desc.nVendor == 0xFFFF) continue;

                PrintFormat("Bus: %d, Slot: %d, Func: %d\n", nBus, nSlot, nFunction);
                PrintFormat("Interface: %d, Class: %d, Subclass: %d\n", desc.nInterface, desc.nClass, desc.nSubclass);
            }
            
        }
    
    PrintString("Finished scanning.");
    
}
