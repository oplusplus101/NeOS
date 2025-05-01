
#ifndef __PCI_H
#define __PCI_H

#include <KNeOS.h>

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

#endif // __PCI_H
