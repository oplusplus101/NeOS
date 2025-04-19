
#ifndef __SYSCALLS_H
#define __SYSCALLS_H

#include <common/types.h>
#include <process.h>

typedef struct
{
    QWORD qwCode; // The value of RAX.
    BYTE nRing; // To ensure that a user-mode process cannot make a kernel-level call (e.g. a user-mode process tries to read directly from a disk, which could be a security issue)
    void (*pCallback)(sCPUState *);
} sSyscallRegistryEntry;

BOOL RegisterSyscall(QWORD qwCode, BYTE nRing, void (*pCallback)(sCPUState *));
void InitSyscalls();

#endif // __SYSCALLS_H
