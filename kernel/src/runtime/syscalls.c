
#include <runtime/syscalls.h>
#include <archimports.h>
#include <neos.h>
#include <io/io.h>

QWORD SYSV SyscallHandler(sSyscallFrame *pFrame)
{
    GetCurrentProcess();
    
    switch (pFrame->qwSyscallNumber)
    {
    case 0: // NeoCreateFile
        // return CreateFile()
        break;
    case 1: // NeoReadFile
        break;
    case 2: // NeoWriteFile
        break;
    default:
        break;
    }

    return NEOS_FAILURE;
}

void InitialiseSyscalls()
{
    ArchRegisterSyscallHandler(SyscallHandler);
}
