
#define ARCH_EXE
#include <hardware/apic.h>
#include <hardware/ports.h>
#include <io/log.h>
#include <cpuid.h>
#include <common/panic.h>
#include <hardware/pic.h>

bool _CheckAPIC()
{
    QWORD qwRAX, qwRDX;
    SimpleCPUID(1, &qwRAX, &qwRDX);

    return qwRDX & CPUID_FEAT_EDX_APIC;
}

void _DisablePIC()
{
    outb(0x20, 0xFF);
    outb(0xA0, 0xFF);
}

void _InitialiseAPIC(sARCHHeader *pHeader)
{
    _InitialisePIC(pHeader);
    // _DisablePIC(); // Disable the legacy PIC
    // _ASSERT(_CheckAPIC(), L"APIC not found on this device");

}

void ArchEndOfInterrupt(BYTE bInterrupt)
{
    if (bInterrupt >= 0x28)
    {
        outb(PIC_SLAVE_COMMAND, 0x20);
        IOWait();
    }

    outb(PIC_MASTER_COMMAND, 0x20);
    IOWait();
}
