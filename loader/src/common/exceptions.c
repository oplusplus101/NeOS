
#include <common/exceptions.h>
#include <common/log.h>
#include <common/panic.h>
#include <common/screen.h>
#include <common/types.h>

#include <hardware/idt.h>

void PrintStack(QWORD qwRSP)
{
    PrintBytes((PVOID) qwRSP, 512, 32, true);
}

void PrintRegs(QWORD qwRSP)
{
    sCPUState *pState = (sCPUState *) qwRSP; 
    PrintFormat(L"RAX=%p RBX=%p RCX=%p RDX=%p\n", pState->qwRAX, pState->qwRBX, pState->qwRCX, pState->qwRDX, pState->qwRSI, pState->qwRDI);
    PrintFormat(L"RSI=%p RDI=%p RBP=%p RSP=%p\n", pState->qwRSI, pState->qwRDI, pState->qwRBP, pState->qwRSP);
    PrintFormat(L"R8 =%p R9 =%p R10=%p R11=%p\n", pState->qwR8 , pState->qwR9 , pState->qwR10, pState->qwR11); 
    PrintFormat(L"R12=%p R13=%p R14=%p R15=%p\n", pState->qwR12, pState->qwR13, pState->qwR14, pState->qwR15); 
    PrintFormat(L"CS=%p SS=%p\n", pState->qwCS, pState->qwSS);
    PrintFormat(L"RFLAGS=%08X [", pState->qwFlags);
    PCHAR sFlagLetters = "C-P-A-ZSTIDOINMR";

    for (SHORT i = 15; i >= 0; i--)
    {
        if (i == 1 || i == 3 || i == 5) continue;
        PrintChar(pState->qwFlags & (1 << i) ? sFlagLetters[i] : '-');
    }
    PrintFormat(L"] Error=%p\n", pState->qwError);
}

QWORD SYSV Exception0(QWORD qwRSP, WORD wErrorCode)
{
    _KERNEL_PANIC(L"Division by zero");
    return qwRSP;
}

QWORD SYSV Exception1(QWORD qwRSP, WORD wErrorCode)
{
    _KERNEL_PANIC(L"Debug");
    return qwRSP;
}

QWORD SYSV Exception2(QWORD qwRSP, WORD wErrorCode)
{
    _KERNEL_PANIC(L"Non-maskable interrupt");
    return qwRSP;
}

QWORD SYSV Exception3(QWORD qwRSP, WORD wErrorCode)
{
    _KERNEL_PANIC(L"Breakpoint");
    return qwRSP;
}

QWORD SYSV Exception4(QWORD qwRSP, WORD wErrorCode)
{
    _KERNEL_PANIC(L"Overflow");
    return qwRSP;
}

QWORD SYSV Exception5(QWORD qwRSP, WORD wErrorCode)
{
    _KERNEL_PANIC(L"Bound range error");
    return qwRSP;
}

QWORD SYSV Exception6(QWORD qwRSP, WORD wErrorCode)
{
    sCPUState *pState = (sCPUState *) qwRSP;
    PrintBytes((PVOID) pState->qwRIP, 512, 32, true);
    _KERNEL_PANIC(L"Invalid opcode");
    return qwRSP;
}

QWORD SYSV Exception7(QWORD qwRSP, WORD wErrorCode)
{
    _KERNEL_PANIC(L"Device not available");
    return qwRSP;
}

QWORD SYSV Exception8(QWORD qwRSP, WORD wErrorCode)
{
    _KERNEL_PANIC(L"Double fault");
    return qwRSP;
}

QWORD SYSV Exception10(QWORD qwRSP, WORD wErrorCode)
{
    _KERNEL_PANIC(L"Invalid TSS, Error Code: %d", wErrorCode);
    return qwRSP;
}

QWORD SYSV Exception11(QWORD qwRSP, WORD wErrorCode)
{
    _KERNEL_PANIC(L"Segment not present, Error Code: %d", wErrorCode);
    return qwRSP;
}

QWORD SYSV Exception12(QWORD qwRSP, WORD wErrorCode)
{
    _KERNEL_PANIC(L"Stack segment fault, Error Code: %d", wErrorCode);
    return qwRSP;
}

QWORD SYSV Exception13(QWORD qwRSP, WORD wErrorCode)
{
    PrintRegs(qwRSP);
    _KERNEL_PANIC(L"General protection fault, Error Code: %d", wErrorCode);
    return qwRSP;
}

QWORD SYSV Exception14(QWORD qwRSP, WORD wErrorCode)
{
    PrintRegs(qwRSP);
    QWORD nExceptionAddress;
    __asm__ volatile("mov %%cr2, %0" : "=r" (nExceptionAddress));
    
    switch (wErrorCode & 7)
    {
    case 0b000:
        _KERNEL_PANIC(L"Supervisory process tried to read a non-present page entry, Address: 0x%p, Error Code: %d", nExceptionAddress, wErrorCode);
    case 0b001:
        _KERNEL_PANIC(L"Supervisory process tried to read a page and caused a protection fault, Address: 0x%p, Error Code: %d", nExceptionAddress, wErrorCode);
    case 0b010:
        _KERNEL_PANIC(L"Supervisory process tried to write to a non-present page entry, Address: 0x%p, Error Code: %d", nExceptionAddress, wErrorCode);
    case 0b011:
        _KERNEL_PANIC(L"Supervisory process tried to write a page and caused a protection fault, Address: 0x%p, Error Code: %d", nExceptionAddress, wErrorCode);
    case 0b100:
        _KERNEL_PANIC(L"User process tried to read a non-present page entry, Address: 0x%p, Error Code: %d", nExceptionAddress, wErrorCode);
    case 0b101:
        _KERNEL_PANIC(L"User process tried to read a page and caused a protection fault, Address: 0x%p, Error Code: %d", nExceptionAddress, wErrorCode);
    case 0b110:
        _KERNEL_PANIC(L"User process tried to write to a non-present page entry, Address: 0x%p, Error Code: %d", nExceptionAddress, wErrorCode);
    case 0b111:
        _KERNEL_PANIC(L"User process tried to write a page and caused a protection fault, Address: 0x%p, Error Code: %d", nExceptionAddress, wErrorCode);
    }
    return qwRSP;
}

QWORD SYSV Exception16(QWORD qwRSP, WORD wErrorCode)
{
    _KERNEL_PANIC(L"x87 Floating point error");
    return qwRSP;
}

QWORD SYSV Exception17(QWORD qwRSP, WORD wErrorCode)
{
    _KERNEL_PANIC(L"Alignment check, Error Code: %d", wErrorCode);
    return qwRSP;
}

QWORD SYSV Exception18(QWORD qwRSP, WORD wErrorCode)
{
    _KERNEL_PANIC(L"Machine check");
    return qwRSP;
}

QWORD SYSV Exception19(QWORD qwRSP, WORD wErrorCode)
{
    _KERNEL_PANIC(L"SIMD Floating point error");
    return qwRSP;
}

QWORD SYSV Exception20(QWORD qwRSP, WORD wErrorCode)
{
    _KERNEL_PANIC(L"Virtualization error");
    return qwRSP;
}

QWORD SYSV Exception21(QWORD qwRSP, WORD wErrorCode)
{
    _KERNEL_PANIC(L"Control protection error, Error Code: %d", wErrorCode);
    return qwRSP;
}

QWORD SYSV Exception28(QWORD qwRSP, WORD wErrorCode)
{
    _KERNEL_PANIC(L"Hypervisor injection error");
    return qwRSP;
}

QWORD SYSV Exception29(QWORD qwRSP, WORD wErrorCode)
{
    _KERNEL_PANIC(L"VMM communication error, Error Code: %d", wErrorCode);
    return qwRSP;
}

QWORD SYSV Exception30(QWORD qwRSP, WORD wErrorCode)
{
    _KERNEL_PANIC(L"Security error, Error Code: %d", wErrorCode);
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
