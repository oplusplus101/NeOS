
#include "PCI.h"

sList g_lstDevices;

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

// Clears the device list and scans for devices
FUNC_EXPORT void NeoReScan()
{
    ClearList(&g_lstDevices);

    for (INT iBus = 0; iBus < 8; iBus++)
        for (INT iSlot = 0; iSlot < 32; iSlot++)
        {
            INT iFunctions = PCIRead(iBus, iSlot, 0, 14) ? 8 : 1;
            for (INT iFunction = 0; iFunction < iFunctions; iFunction++)
            {
                sPCIDeviceDescriptor sDesc = GetDeviceDescriptor(iBus, iSlot, iFunction);
                if (sDesc.nVendor == 0 || sDesc.nVendor == 0xFFFF) continue;

                AddListElement(&g_lstDevices, &sDesc);
            }
        }
}

// Returns false if the device does not exist
FUNC_EXPORT BOOL KNeoGetPCIDevice(BYTE nClass, BYTE nSubclass, sPCIDeviceDescriptor *pResult)
{
    for (INT i = 0; i < g_lstDevices->qwLength; i++)
    {
        sPCIDeviceDescriptor *pDesc = GetListElement(&g_lstDevices, i);
        if (pDesc->nClass == nClass && pDesc->nSubclass == nSubclass)
        {
            memcpy(pResult, pDesc, sizeof(sPCIDeviceDescriptor));
            return true;
        }
    }

    return false;
}

FUNC_EXPORT sBaseAddressRegister KNeoGetBaseAddressRegister(sPCIDeviceDescriptor sDesc, BYTE nBAR)
{
    sBaseAddressRegister bar = { 0 };
    
    BYTE nHeaderType = PCIRead(sDesc.nBus, sDesc.nSlot, sDesc.nFunction, 0x0E) & 0x7F;
    BYTE nMaxBARs    = 6 - (nHeaderType * 4);
    if (nBAR >= nMaxBARs) return bar;

    DWORD dwBARValue = PCIRead(sDesc.nBus, sDesc.nSlot, sDesc.nFunction, 0x10 + nBAR * 4);
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
                DWORD dwBARValueHigh = PCIRead(sDesc.nBus, sDesc.nSlot, sDesc.nFunction, 0x10 + (nBAR + 1) * 4);
                bar.qwAddress = (dwBARValue & 0xFFFFFFF0) | ((QWORD) dwBARValueHigh << 32);
                break;
        }
    }

    return bar;
}

void DriverEntry()
{
    g_lstDevices = CreateEmptyList(sizeof(sPCIDeviceDescriptor));
}
