
#ifndef __SYSCALLIMPL_H
#define __SYSCALLIMPL_H

#include <process.h>

void Syscall_KNeoPrintString(sCPUState *pCPUState);
void Syscall_KNeoKernelPanic(sCPUState *pCPUState);
void Syscall_KNeoGetCursorPosition(sCPUState *pCPUState);
void Syscall_KNeoSetCursorPosition(sCPUState *pCPUState);
void Syscall_KNeoClearScreen(sCPUState *pCPUState);
void Syscall_KNeoGetDriver(sCPUState *pCPUState);
void Syscall_KNeoGetModule(sCPUState *pCPUState);
void Syscall_KNeoGetDriverFunction(sCPUState *pCPUState);
void Syscall_KNeoGetModuleFunction(sCPUState *pCPUState);
void Syscall_KNeoMapPagesToIdentity(sCPUState *pCPUState);
void Syscall_KNeoMapPages(sCPUState *pCPUState);
void Syscall_KNeoAllocatePages(sCPUState *pCPUState);
void Syscall_KNeoFreePages(sCPUState *pCPUState);
void Syscall_KNeoAllocateHeap(sCPUState *pCPUState);
void Syscall_KNeoReAllocateHeap(sCPUState *pCPUState);
void Syscall_KNeoFreeHeap(sCPUState *pCPUState);
void Syscall_KNeoRegisterSyscall(sCPUState *pCPUState);
void Syscall_KNeoClearSyscall(sCPUState *pCPUState);

#endif // __SYSCALLIMPL_H
