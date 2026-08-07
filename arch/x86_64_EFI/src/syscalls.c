
#include <syscalls.h>
#include <memory/gdt.h>
#include <msr.h>
#include <context.h>
#include <io/screen.h>

extern sGlobalDescriptorTable g_sGDT __attribute__((aligned(16)));

QWORD (SYSV *g_pSyscallHandler)(sSyscallFrame *pFrame);
extern void HandleSyscallStub();

void _InitialiseSyscalls()
{
    QWORD qwEFER = rdmsr(0xC0000080);
    qwEFER |= 1;
    wrmsr(0xC0000080, qwEFER);

    // IA32_STAR
    wrmsr(0xC0000081, ((QWORD) USER_CODE_SEGMENT << 48) |
                      ((QWORD) KERNEL_CODE_SEGMENT << 32));
    // IA32_LSTAR
    wrmsr(0xC0000082, (QWORD) HandleSyscallStub);
    wrmsr(0xC0000084, 0x44700);
    g_pSyscallHandler = NULL;
}


QWORD SYSV HandleSyscall(sSyscallFrame *pFrame)
{
    if (g_pSyscallHandler == NULL) return 0;
    return g_pSyscallHandler(pFrame);
}


void SYSV ArchRegisterSyscallHandler(QWORD (SYSV *pHandler)(sSyscallFrame *pFrame))
{
    g_pSyscallHandler = pHandler;
}
