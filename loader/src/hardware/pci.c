
#include <hardware/pci.h>
#include <hardware/ports.h>
#include <common/screen.h>
#include <hardware/storage/nvme.h>
#include <hardware/storage/drive.h>
#include <hardware/storage/ahci.h>

DWORD PCIRead(BYTE nBus, BYTE nSlot, BYTE nFunction, BYTE nOffset)
{
  
    DWORD dwAddress = ((DWORD) nBus << 16) | ((DWORD) nSlot << 11) |
                     ((DWORD) nFunction << 8) | ((DWORD) nOffset & 0xFC) | 0x80000000;

    outl(0xCF8, dwAddress);
    return inl(0xCFC) >> ((nOffset & 3) << 3);
}

void PCIWrite(BYTE nBus, BYTE nSlot, BYTE nFunction, BYTE nOffset, DWORD dwValue)
{
  
    DWORD dwAddress = ((DWORD) nBus << 16) | ((DWORD) nSlot << 11) |
                     ((DWORD) nFunction << 8) | ((DWORD) nOffset & 0xFC) | 0x80000000;

    outl(0xCF8, dwAddress);
    outl(0xCFC, dwValue);
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

sBaseAddressRegister GetBaseAddressRegister(BYTE nBus, BYTE nSlot, BYTE nFunction, BYTE nBAR)
{
    sBaseAddressRegister bar = { 0 };
    
    BYTE nHeaderType = PCIRead(nBus, nSlot, nFunction, 0x0E) & 0x7F;
    BYTE nMaxBARs    = 6 - (nHeaderType * 4);
    if (nBAR >= nMaxBARs) return bar;

    DWORD dwBARValue = PCIRead(nBus, nSlot, nFunction, 0x10 + nBAR * 4);
    bar.eType = dwBARValue & 0x01 ? PCI_IO : PCI_MMAP;

    if (bar.eType == PCI_IO)
    {
        bar.qwAddress = dwBARValue & 0xFFFFFFFC;
        bar.bPrefetchable = false;
    }
    else if (bar.eType == PCI_MMAP)
    {
        switch ((dwBARValue >> 1) & 0x03)
        {
            case 0: // 32 Bit Mode
                bar.qwAddress = dwBARValue & 0xFFFFFFF0;
                break;
            case 1: // 20 Bit Mode
                break;
            case 2: // 64 Bit Mode
                DWORD dwBARValueHigh = PCIRead(nBus, nSlot, nFunction, 0x10 + (nBAR + 1) * 4);
                bar.qwAddress = (dwBARValue & 0xFFFFFFF0) | ((QWORD) dwBARValueHigh << 32);
                break;
        }
    }

    return bar;
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

                switch (desc.nClass)
                {
                case 0x01: // Mass storage device
                    switch (desc.nSubclass)
                    {
                    case 0x06: // AHCI
                        SetupAHCI(nBus, nSlot, nFunction);
                        break;
                    case 0x08: // NVMe
                        SetupNVMe(nBus, nSlot, nFunction);
                        break;
                    }
                    break;
                }

                PrintFormat(L"Bus: %d, Slot: %d, Func: %d\n", nBus, nSlot, nFunction);
                PrintFormat(L"Interface: %d, Class: %d, Subclass: %d\n", desc.nInterface, desc.nClass, desc.nSubclass);
            }
        }
    
    PrintString("Finished scanning.\n");
}
