
#include <common/exceptions.h>
#include <common/panic.h>
#include <hardware/idt.h>

void PrintStack(QWORD qwRSP)
{
    PrintBytes((PVOID) qwRSP, 512, 32, true);
}

QWORD Exception0(QWORD qwRSP, BYTE nErrorCode)
{
    _KERNEL_PANIC("Division by zero");
    return qwRSP;
}

QWORD Exception1(QWORD qwRSP, BYTE nErrorCode)
{
    _KERNEL_PANIC("Debug");
    return qwRSP;
}

QWORD Exception2(QWORD qwRSP, BYTE nErrorCode)
{
    _KERNEL_PANIC("Non-maskable interrupt");
    return qwRSP;
}

QWORD Exception3(QWORD qwRSP, BYTE nErrorCode)
{
    _KERNEL_PANIC("Breakpoint");
    return qwRSP;
}

QWORD Exception4(QWORD qwRSP, BYTE nErrorCode)
{
    _KERNEL_PANIC("Overflow");
    return qwRSP;
}

QWORD Exception5(QWORD qwRSP, BYTE nErrorCode)
{
    _KERNEL_PANIC("Bound range error");
    return qwRSP;
}

QWORD Exception6(QWORD qwRSP, BYTE nErrorCode)
{
    _KERNEL_PANIC("Invalid opcode");
    return qwRSP;
}

QWORD Exception7(QWORD qwRSP, BYTE nErrorCode)
{
    _KERNEL_PANIC("Device not available");
    return qwRSP;
}

QWORD Exception8(QWORD qwRSP, BYTE nErrorCode)
{
    _KERNEL_PANIC("Double fault");
    return qwRSP;
}

QWORD Exception10(QWORD qwRSP, BYTE nErrorCode)
{
    _KERNEL_PANIC_EC(nErrorCode, "Invalid TSS");
    return qwRSP;
}

QWORD Exception11(QWORD qwRSP, BYTE nErrorCode)
{
    _KERNEL_PANIC_EC(nErrorCode, "Segment not present");
    return qwRSP;
}

QWORD Exception12(QWORD qwRSP, BYTE nErrorCode)
{
    _KERNEL_PANIC_EC(nErrorCode, "Stack segment fault");
    return qwRSP;
}

QWORD Exception13(QWORD qwRSP, BYTE nErrorCode)
{
    PrintStack(qwRSP);
    _KERNEL_PANIC_EC(nErrorCode, "General protection fault");
    return qwRSP;
}

QWORD Exception14(QWORD qwRSP, BYTE nErrorCode)
{
    QWORD nExceptionAddress;
    __asm__ volatile("mov %%cr2, %0" : "=r" (nExceptionAddress));

    switch (nErrorCode & 7)
    {
    case 0b000:
        _KERNEL_PANIC_EC(nErrorCode, "Supervisory process tried to read a non-present page entry\nAddress: 0x%16X", nExceptionAddress);
    case 0b001:
        _KERNEL_PANIC_EC(nErrorCode, "Supervisory process tried to read a page and caused a protection fault\nAddress: 0x%16X", nExceptionAddress);
    case 0b010:
        _KERNEL_PANIC_EC(nErrorCode, "Supervisory process tried to write to a non-present page entry\nAddress: 0x%16X", nExceptionAddress);
    case 0b011:
        _KERNEL_PANIC_EC(nErrorCode, "Supervisory process tried to write a page and caused a protection fault\nAddress: 0x%16X", nExceptionAddress);
    case 0b100:
        _KERNEL_PANIC_EC(nErrorCode, "User process tried to read a non-present page entry\nAddress: 0x%16X", nExceptionAddress);
    case 0b101:
        _KERNEL_PANIC_EC(nErrorCode, "User process tried to read a page and caused a protection fault\nAddress: 0x%16X", nExceptionAddress);
    case 0b110:
        _KERNEL_PANIC_EC(nErrorCode, "User process tried to write to a non-present page entry\nAddress: 0x%16X", nExceptionAddress);
    case 0b111:
        _KERNEL_PANIC_EC(nErrorCode, "User process tried to write a page and caused a protection fault\nAddress: 0x%16X", nExceptionAddress);
    }
    return qwRSP;
}

QWORD Exception16(QWORD qwRSP, BYTE nErrorCode)
{
    _KERNEL_PANIC("x87 Floating point error");
    return qwRSP;
}

QWORD Exception17(QWORD qwRSP, BYTE nErrorCode)
{
    _KERNEL_PANIC_EC(nErrorCode, "Alignment check");
    return qwRSP;
}

QWORD Exception18(QWORD qwRSP, BYTE nErrorCode)
{
    _KERNEL_PANIC("Machine check");
    return qwRSP;
}

QWORD Exception19(QWORD qwRSP, BYTE nErrorCode)
{
    _KERNEL_PANIC("SIMD Floating point error");
    return qwRSP;
}

QWORD Exception20(QWORD qwRSP, BYTE nErrorCode)
{
    _KERNEL_PANIC("Virtualization error");
    return qwRSP;
}

QWORD Exception21(QWORD qwRSP, BYTE nErrorCode)
{
    _KERNEL_PANIC_EC(nErrorCode, "Control protection error");
    return qwRSP;
}

QWORD Exception28(QWORD qwRSP, BYTE nErrorCode)
{
    _KERNEL_PANIC("Hypervisor injection error");
    return qwRSP;
}

QWORD Exception29(QWORD qwRSP, BYTE nErrorCode)
{
    _KERNEL_PANIC_EC(nErrorCode, "VMM communication error");
    return qwRSP;
}

QWORD Exception30(QWORD qwRSP, BYTE nErrorCode)
{
    _KERNEL_PANIC_EC(nErrorCode, "Security error");
    return qwRSP;
}

void RegisterExceptions()
{
    RegisterException(0,  Exception0);  // Division by 0
    RegisterException(1,  Exception1);  // Debug
    RegisterException(2,  Exception2);  // Non-maskable interrupt
    RegisterException(3,  Exception3);  // Breakpoint
    RegisterException(4,  Exception4);  // Overflow
    RegisterException(5,  Exception5);  // Bound range exceeded
    RegisterException(6,  Exception6);  // Invalid opcode
    RegisterException(7,  Exception7);  // Device not available
    RegisterException(8,  Exception8);  // Double fault
    RegisterException(10, Exception10); // Invalid TSS
    RegisterException(11, Exception11); // Segment not present
    RegisterException(12, Exception12); // Stack segment fault
    RegisterException(13, Exception13); // General protection fault
    RegisterException(14, Exception14); // Page fault
    RegisterException(16, Exception16); // x87 floating point error
    RegisterException(17, Exception17); // Alignment check
    RegisterException(18, Exception18); // Machine check
    RegisterException(19, Exception19); // SIMD floating point error
    RegisterException(20, Exception20); // Virtualization error
    RegisterException(21, Exception21); // Control protection error
    RegisterException(28, Exception28); // Hypervisor injection error
    RegisterException(29, Exception29); // VMM communication error
    RegisterException(30, Exception30); // Security error
}
