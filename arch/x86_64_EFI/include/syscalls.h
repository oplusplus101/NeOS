
#ifndef __SYSCALLS_H
#define __SYSCALLS_H


#include <common/types.h>

typedef struct
{
    QWORD qwSyscallNumber;
    QWORD qwParam0;
    QWORD qwParam1;
    QWORD qwParam2;
    QWORD qwParam3;
    QWORD qwParam4;
    QWORD qwParam5;
    QWORD qwParam6;
} __attribute((packed)) sSyscallFrame;

#ifdef ARCH_EXE

void _InitialiseSyscalls();

#endif // ARCH_EXE

void SYSV ArchRegisterSyscallHandler(QWORD (SYSV *pHandler)(sSyscallFrame *pFrame));


#endif // __SYSCALLS_H
