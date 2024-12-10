
#include <common/exceptions.h>
#include <common/panic.h>
#include <hardware/idt.h>

size_t Exception0(size_t nRSP, uint8_t nErrorCode)
{
    _KernelPanic("Division by zero");
    return nRSP;
}

size_t Exception5(size_t nRSP, uint8_t nErrorCode)
{
    _KernelPanic("Bound range error");
    return nRSP;
}

size_t Exception6(size_t nRSP, uint8_t nErrorCode)
{
    _KernelPanic("Invalid opcode");
    return nRSP;
}

size_t Exception7(size_t nRSP, uint8_t nErrorCode)
{
    _KernelPanic("Device not available");
    return nRSP;
}

size_t Exception8(size_t nRSP, uint8_t nErrorCode)
{
    _KernelPanic("Double fault");
    return nRSP;
}

size_t Exception10(size_t nRSP, uint8_t nErrorCode)
{
    _KernelPanicEC(nErrorCode, "Invalid TSS");
    return nRSP;
}

size_t Exception11(size_t nRSP, uint8_t nErrorCode)
{
    _KernelPanicEC(nErrorCode, "Segment not present");
    return nRSP;
}

size_t Exception12(size_t nRSP, uint8_t nErrorCode)
{
    _KernelPanicEC(nErrorCode, "Stack segment fault");
    return nRSP;
}

size_t Exception13(size_t nRSP, uint8_t nErrorCode)
{
    _KernelPanicEC(nErrorCode, "General protection fault");
    return nRSP;
}

size_t Exception14(size_t rsp, uint8_t nErrorCode)
{
    size_t nExceptionAddress;
    __asm__ volatile("mov %%cr2, %0" : "=r" (nExceptionAddress));

    switch (nErrorCode & 7)
    {
    case 0b000:
        _KernelPanicEC(nErrorCode, "Supervisory process tried to read a non-present page entry\nAddress: 0x%16X", nExceptionAddress);
    case 0b001:
        _KernelPanicEC(nErrorCode, "Supervisory process tried to read a page and caused a protection fault\nAddress: 0x%16X", nExceptionAddress);
    case 0b010:
        _KernelPanicEC(nErrorCode, "Supervisory process tried to write to a non-present page entry\nAddress: 0x%16X", nExceptionAddress);
    case 0b011:
        _KernelPanicEC(nErrorCode, "Supervisory process tried to write a page and caused a protection fault\nAddress: 0x%16X", nExceptionAddress);
    case 0b100:
        _KernelPanicEC(nErrorCode, "User process tried to read a non-present page entry\nAddress: 0x%16X", nExceptionAddress);
    case 0b101:
        _KernelPanicEC(nErrorCode, "User process tried to read a page and caused a protection fault\nAddress: 0x%16X", nExceptionAddress);
    case 0b110:
        _KernelPanicEC(nErrorCode, "User process tried to write to a non-present page entry\nAddress: 0x%16X", nExceptionAddress);
    case 0b111:
        _KernelPanicEC(nErrorCode, "User process tried to write a page and caused a protection fault\nAddress: 0x%16X", nExceptionAddress);
    }
    return rsp;
}

size_t Exception16(size_t rsp, uint8_t nErrorCode)
{
    _KernelPanic("x87 Floating point error");
    return rsp;
}

size_t Exception17(size_t rsp, uint8_t nErrorCode)
{
    _KernelPanicEC(nErrorCode, "Alignment check");
    return rsp;
}

size_t Exception19(size_t rsp, uint8_t nErrorCode)
{
    _KernelPanic("SIMD Floating point error");
    return rsp;
}

size_t Exception20(size_t rsp, uint8_t nErrorCode)
{
    _KernelPanic("Virtualization error");
    return rsp;
}

size_t Exception21(size_t rsp, uint8_t nErrorCode)
{
    _KernelPanicEC(nErrorCode, "Control protection error");
    return rsp;
}

size_t Exception28(size_t rsp, uint8_t nErrorCode)
{
    _KernelPanic("Hypervisor injection error");
    return rsp;
}

size_t Exception29(size_t rsp, uint8_t nErrorCode)
{
    _KernelPanicEC(nErrorCode, "VMM communication error");
    return rsp;
}

size_t Exception30(size_t rsp, uint8_t nErrorCode)
{
    _KernelPanicEC(nErrorCode, "Security error");
    return rsp;
}

void RegisterExceptions()
{
    RegisterException(0,  Exception0); // Division by 0
    RegisterException(5,  Exception5); // Bound range exceeded
    RegisterException(6,  Exception6); // Invalid opcode
    RegisterException(7,  Exception7); // Device not available
    RegisterException(8,  Exception8); // Double fault
    RegisterException(10, Exception10); // Invalid TSS
    RegisterException(11, Exception11); // Segment not present
    RegisterException(12, Exception12); // Stack segment fault
    RegisterException(13, Exception13); // General protection fault
    RegisterException(14, Exception14); // Page fault
    RegisterException(16, Exception16); // x87 floating point error
    RegisterException(17, Exception17); // Alignment check
    RegisterException(19, Exception19); // SIMD floating point error
    RegisterException(20, Exception20); // Virtualization error
    RegisterException(21, Exception21); // Control protection error
    RegisterException(28, Exception28); // Hypervisor injection error
    RegisterException(29, Exception29); // VMM communication error
    RegisterException(30, Exception30); // Security error
}
