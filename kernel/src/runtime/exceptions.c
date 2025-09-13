
#include <runtime/exceptions.h>
#include <runtime/process.h>
#include <common/panic.h>

typedef struct _tagStackFrame
{
    struct _tagStackFrame *pRBP;
    QWORD qwRIP;
} __attribute__((packed)) sStackFrame;

void PrintRegs(QWORD qwRSP);

#define _PRINT_FAULT(qwRSP, ...) \
    { PrintRegs(qwRSP); \
    _KERNEL_PANIC(__VA_ARGS__); }


void TraceStack(DWORD dwMaxFrames)
{
    sStackFrame *pStack = NULL;
    __asm__ volatile ("mov %%rbp, %0" : "=r"(pStack));
    PrintFormat(L"Stack trace:\n");
    for(QWORD qwFrame = 0; pStack && qwFrame < dwMaxFrames; qwFrame++)
    {
        PrintFormat(L"  0x%16X\n", pStack->qwRIP);
        pStack = pStack->pRBP;
    }
}

void KillCurrentProcess(PWCHAR wszReason)
{
    KillProcess(GetCurrentPID(), wszReason);
}

QWORD KException0(QWORD qwRSP, BYTE nErrorCode)
{
    if (GetCurrentPID() == -1)
        _PRINT_FAULT(qwRSP, L"Division by zero");
        
    KillCurrentProcess(L"Division by zero");

    return ScheduleProcesses(qwRSP);
}

QWORD KException1(QWORD qwRSP, BYTE nErrorCode)
{
    if (GetCurrentPID() == -1)
        _PRINT_FAULT(qwRSP, L"Debug");
        
    KillCurrentProcess(L"Debug");

    return ScheduleProcesses(qwRSP);
}

QWORD KException2(QWORD qwRSP, BYTE nErrorCode)
{
    if (GetCurrentPID() == -1)
        _PRINT_FAULT(qwRSP, L"Non-maskable interrupt");
        
    KillCurrentProcess(L"Non-maskable interrupt");

    return ScheduleProcesses(qwRSP);
}

QWORD KException3(QWORD qwRSP, BYTE nErrorCode)
{
    if (GetCurrentPID() == -1)
        _PRINT_FAULT(qwRSP, L"Breakpoint");
        
    KillCurrentProcess(L"Breakpoint");

    return ScheduleProcesses(qwRSP);
}

QWORD KException4(QWORD qwRSP, BYTE nErrorCode)
{
    if (GetCurrentPID() == -1)
        _PRINT_FAULT(qwRSP, L"Overflow");
        
    KillCurrentProcess(L"Overflow");

    return ScheduleProcesses(qwRSP);
}

QWORD KException5(QWORD qwRSP, BYTE nErrorCode)
{
    if (GetCurrentPID() == -1)
        _PRINT_FAULT(qwRSP, L"Bound range error");
        
    KillCurrentProcess(L"Bound range error");

    return ScheduleProcesses(qwRSP);
}

QWORD KException6(QWORD qwRSP, BYTE nErrorCode)
{
    if (GetCurrentPID() == -1)
        _PRINT_FAULT(qwRSP, L"Invalid opcode");
    
    KillCurrentProcess(L"Invalid opcode");

    return ScheduleProcesses(qwRSP);
}

QWORD KException7(QWORD qwRSP, BYTE nErrorCode)
{
    if (GetCurrentPID() == -1)
        _PRINT_FAULT(qwRSP, L"Device not available");
        
    KillCurrentProcess(L"Device not available");

    return ScheduleProcesses(qwRSP);
}

QWORD KException8(QWORD qwRSP, BYTE nErrorCode)
{
    if (GetCurrentPID() == -1)
        _PRINT_FAULT(qwRSP, L"Double fault");
        
    KillCurrentProcess(L"Double fault");

    return ScheduleProcesses(qwRSP);
}

QWORD KException10(QWORD qwRSP, BYTE nErrorCode)
{
    if (GetCurrentPID() == -1)
        _PRINT_FAULT(qwRSP, L"Invalid TSS");
        
    KillCurrentProcess(L"Invalid TSS");

    return ScheduleProcesses(qwRSP);
}

QWORD KException11(QWORD qwRSP, BYTE nErrorCode)
{
    if (GetCurrentPID() == -1)
        _PRINT_FAULT(qwRSP, L"Segment not present");
        
    KillCurrentProcess(L"Segment not present");

    return ScheduleProcesses(qwRSP);
}

QWORD KException12(QWORD qwRSP, BYTE nErrorCode)
{
    if (GetCurrentPID() == -1)
        _PRINT_FAULT(qwRSP, L"Stack segment fault");
        
    KillCurrentProcess(L"Stack segment fault");

    return ScheduleProcesses(qwRSP);
}

QWORD KException13(QWORD qwRSP, BYTE nErrorCode)
{
    QWORD qwExceptionAddress;
    __asm__ volatile("mov %%cr2, %0" : "=r" (qwExceptionAddress));
    PrintFormat(L"Address: 0x%p\n", qwExceptionAddress);
    if (GetCurrentPID() == -1)
        _PRINT_FAULT(qwRSP, L"General protection fault");
        
    KillCurrentProcess(L"General protection fault");

    return ScheduleProcesses(qwRSP);
}

QWORD KException14(QWORD qwRSP, BYTE nErrorCode)
{
    QWORD qwExceptionAddress;
    __asm__ volatile("mov %%cr2, %0" : "=r" (qwExceptionAddress));
    
    if (GetCurrentPID() == -1)
        switch (nErrorCode & 7)
        {
        case 0b000:
            _PRINT_FAULT(qwRSP, L"Supervisory process tried to read a non-present page entry\nAddress: 0x%p\nError Code: %d", qwExceptionAddress, nErrorCode);
        case 0b001:
            _PRINT_FAULT(qwRSP, L"Supervisory process tried to read a page and caused a protection fault\nAddress: 0x%p\nError Code: %d", qwExceptionAddress, nErrorCode);
        case 0b010:
            _PRINT_FAULT(qwRSP, L"Supervisory process tried to write to a non-present page entry\nAddress: 0x%p\nError Code: %d", qwExceptionAddress, nErrorCode);
        case 0b011:
            _PRINT_FAULT(qwRSP, L"Supervisory process tried to write a page and caused a protection fault\nAddress: 0x%p\nError Code: %d", qwExceptionAddress, nErrorCode);
        case 0b100:
            _PRINT_FAULT(qwRSP, L"User process tried to read a non-present page entry\nAddress: 0x%p\nError Code: %d", qwExceptionAddress, nErrorCode);
        case 0b101:
            _PRINT_FAULT(qwRSP, L"User process tried to read a page and caused a protection fault\nAddress: 0x%p\nError Code: %d", qwExceptionAddress, nErrorCode);
        case 0b110:
            _PRINT_FAULT(qwRSP, L"User process tried to write to a non-present page entry\nAddress: 0x%p\nError Code: %d", qwExceptionAddress, nErrorCode);
        case 0b111:
            _PRINT_FAULT(qwRSP, L"User process tried to write a page and caused a protection fault\nAddress: 0x%p\nError Code: %d", qwExceptionAddress, nErrorCode);
        }

    SetFGColor(NEOS_ERROR_COLOR);
    PrintFormat(L"Address: 0x%p\n", qwExceptionAddress);
    PrintFormat(L"PML4 Address: 0x%p\n", GetCurrentPML4());

    switch (nErrorCode & 7)
    {
    case 0b000:
        KillCurrentProcess(L"Supervisory process tried to read a non-present page entry");
    case 0b001:
        KillCurrentProcess(L"Supervisory process tried to read a page and caused a protection fault");
    case 0b010:
        KillCurrentProcess(L"Supervisory process tried to write to a non-present page entry");
    case 0b011:
        KillCurrentProcess(L"Supervisory process tried to write a page and caused a protection fault");
    case 0b100:
        KillCurrentProcess(L"User process tried to read a non-present page entry");
    case 0b101:
        KillCurrentProcess(L"User process tried to read a page and caused a protection fault");
    case 0b110:
        KillCurrentProcess(L"User process tried to write to a non-present page entry");
    case 0b111:
        KillCurrentProcess(L"User process tried to write a page and caused a protection fault");
    }

    return ScheduleProcesses(qwRSP);
}


void RegisterKernelExceptions()
{
    RegisterException(0,  KException0);  // Division by 0
    RegisterException(1,  KException1);  // Debug
    RegisterException(2,  KException2);  // Non-maskable interrupt
    RegisterException(3,  KException3);  // Breakpoint
    RegisterException(4,  KException4);  // Overflow
    RegisterException(5,  KException5);  // Bound range exceeded
    RegisterException(6,  KException6);  // Invalid opcode
    RegisterException(7,  KException7);  // Device not available
    RegisterException(8,  KException8);  // Double fault
    RegisterException(10, KException10); // Invalid TSS
    RegisterException(11, KException11); // Segment not present
    RegisterException(12, KException12); // Stack segment fault
    RegisterException(13, KException13); // General protection fault
    RegisterException(14, KException14); // Page fault
}
