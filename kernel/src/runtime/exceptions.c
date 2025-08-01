
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
    PrintRegs(qwRSP); \
    _KERNEL_PANIC(__VA_ARGS__);


void TraceStack(DWORD dwMaxFrames)
{
    sStackFrame *pStack = NULL;
    __asm__ volatile ("mov %%rbp, %0" : "=r"(pStack));
    PrintFormat("Stack trace:\n");
    for(QWORD qwFrame = 0; pStack && qwFrame < dwMaxFrames; qwFrame++)
    {
        PrintFormat("  0x%16X\n", pStack->qwRIP);
        pStack = pStack->pRBP;
    }
}

void KillCurrentProcess(PWCHAR wszReason)
{
    KillProcess(GetCurrentPID(), wszReason);
    __asm__ volatile ("cli\nhlt");
}

QWORD KException0(QWORD qwRSP, BYTE nErrorCode)
{
    if (GetCurrentPID() == -1)
        _PRINT_FAULT(qwRSP, "Division by zero");
        
    KillCurrentProcess(L"Division by zero");
    return qwRSP;
    // return ScheduleProcesses(qwRSP);
}

QWORD KException1(QWORD qwRSP, BYTE nErrorCode)
{
    if (GetCurrentPID() == -1)
        _PRINT_FAULT(qwRSP, "Debug");
        
    KillCurrentProcess(L"Debug");
    return qwRSP;
    // return ScheduleProcesses(qwRSP);
}

QWORD KException2(QWORD qwRSP, BYTE nErrorCode)
{
    if (GetCurrentPID() == -1)
        _PRINT_FAULT(qwRSP, "Non-maskable interrupt");
        
    KillCurrentProcess(L"Non-maskable interrupt");
    return qwRSP;
    // return ScheduleProcesses(qwRSP);
}

QWORD KException3(QWORD qwRSP, BYTE nErrorCode)
{
    if (GetCurrentPID() == -1)
        _PRINT_FAULT(qwRSP, "Breakpoint");
        
    KillCurrentProcess(L"Breakpoint");
    return qwRSP;
    // return ScheduleProcesses(qwRSP);
}

QWORD KException4(QWORD qwRSP, BYTE nErrorCode)
{
    if (GetCurrentPID() == -1)
        _PRINT_FAULT(qwRSP, "Overflow");
        
    KillCurrentProcess(L"Overflow");
    return qwRSP;
    // return ScheduleProcesses(qwRSP);
}

QWORD KException5(QWORD qwRSP, BYTE nErrorCode)
{
    if (GetCurrentPID() == -1)
        _PRINT_FAULT(qwRSP, "Bound range error");
        
    KillCurrentProcess(L"Bound range error");
    return qwRSP;
    // return ScheduleProcesses(qwRSP);
}

QWORD KException6(QWORD qwRSP, BYTE nErrorCode)
{
    TraceStack(10);
    PrintFormat("RIP: %p\n", ((sCPUState *) qwRSP)->qwRIP);
    if (GetCurrentPID() == -1)
        _PRINT_FAULT(qwRSP, "Invalid opcode");
    
    KillCurrentProcess(L"Invalid opcode");
    return qwRSP;
    // return ScheduleProcesses(qwRSP);
}

QWORD KException7(QWORD qwRSP, BYTE nErrorCode)
{
    if (GetCurrentPID() == -1)
        _PRINT_FAULT(qwRSP, "Device not available");
        
    KillCurrentProcess(L"Device not available");
    return qwRSP;
    // return ScheduleProcesses(qwRSP);
}

QWORD KException8(QWORD qwRSP, BYTE nErrorCode)
{
    if (GetCurrentPID() == -1)
        _PRINT_FAULT(qwRSP, "Double fault");
        
    KillCurrentProcess(L"Double fault");
    return qwRSP;
    // return ScheduleProcesses(qwRSP);
}

QWORD KException10(QWORD qwRSP, BYTE nErrorCode)
{
    if (GetCurrentPID() == -1)
        _PRINT_FAULT(qwRSP, "Invalid TSS");
        
    KillCurrentProcess(L"Invalid TSS");
    return qwRSP;
    // return ScheduleProcesses(qwRSP);
}

QWORD KException11(QWORD qwRSP, BYTE nErrorCode)
{
    if (GetCurrentPID() == -1)
        _PRINT_FAULT(qwRSP, "Segment not present");
        
    KillCurrentProcess(L"Segment not present");
    return qwRSP;
    // return ScheduleProcesses(qwRSP);
}

QWORD KException12(QWORD qwRSP, BYTE nErrorCode)
{
    if (GetCurrentPID() == -1)
        _PRINT_FAULT(qwRSP, "Stack segment fault");
        
    KillCurrentProcess(L"Stack segment fault");
    return qwRSP;
    // return ScheduleProcesses(qwRSP);
}

QWORD KException13(QWORD qwRSP, BYTE nErrorCode)
{
    QWORD qwExceptionAddress;
    __asm__ volatile("mov %%cr2, %0" : "=r" (qwExceptionAddress));
    PrintFormat("Address: %p\n", qwExceptionAddress);
    if (GetCurrentPID() == -1)
        _PRINT_FAULT(qwRSP, "General protection fault");
        
    KillCurrentProcess(L"General protection fault");
    return qwRSP;
    // return ScheduleProcesses(qwRSP);
}

QWORD KException14(QWORD qwRSP, BYTE nErrorCode)
{
    QWORD qwExceptionAddress;
    __asm__ volatile("mov %%cr2, %0" : "=r" (qwExceptionAddress));
    
    if (GetCurrentPID() == -1)
        switch (nErrorCode & 7)
        {
        case 0b000:
            _PRINT_FAULT(qwRSP, "Supervisory process tried to read a non-present page entry\nAddress: 0x%p\nError Code: %d", qwExceptionAddress, nErrorCode);
        case 0b001:
            _PRINT_FAULT(qwRSP, "Supervisory process tried to read a page and caused a protection fault\nAddress: 0x%p\nError Code: %d", qwExceptionAddress, nErrorCode);
        case 0b010:
            _PRINT_FAULT(qwRSP, "Supervisory process tried to write to a non-present page entry\nAddress: 0x%p\nError Code: %d", qwExceptionAddress, nErrorCode);
        case 0b011:
            _PRINT_FAULT(qwRSP, "Supervisory process tried to write a page and caused a protection fault\nAddress: 0x%p\nError Code: %d", qwExceptionAddress, nErrorCode);
        case 0b100:
            _PRINT_FAULT(qwRSP, "User process tried to read a non-present page entry\nAddress: 0x%p\nError Code: %d", qwExceptionAddress, nErrorCode);
        case 0b101:
            _PRINT_FAULT(qwRSP, "User process tried to read a page and caused a protection fault\nAddress: 0x%p\nError Code: %d", qwExceptionAddress, nErrorCode);
        case 0b110:
            _PRINT_FAULT(qwRSP, "User process tried to write to a non-present page entry\nAddress: 0x%p\nError Code: %d", qwExceptionAddress, nErrorCode);
        case 0b111:
            _PRINT_FAULT(qwRSP, "User process tried to write a page and caused a protection fault\nAddress: 0x%p\nError Code: %d", qwExceptionAddress, nErrorCode);
        }

    SetFGColor(NEOS_ERROR_COLOR);
    PrintFormat("Address: 0x%p\n", qwExceptionAddress);
    PrintFormat("PML4 Address: 0x%p\n", GetCurrentPML4());

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
    return qwRSP;
    // return ScheduleProcesses(qwRSP);
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
