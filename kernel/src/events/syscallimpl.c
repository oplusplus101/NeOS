
#include <events/syscallimpl.h>
#include <events/syscalls.h>
#include <common/screen.h>
#include <common/panic.h>
#include <memory/heap.h>

// 0x0000 - 0x000F Print functions

// Code: 0x0000
// RBX is the address of a zero-terminated string
void Syscall_KNeoPrintString(sCPUState *pCPUState)
{
    PrintString((PCHAR) pCPUState->qwRBX);
}

// Code: 0x0001
// RBX is the address of a zero-terminated string
void Syscall_KNeoKernelPanic(sCPUState *pCPUState)
{
    _KERNEL_PANIC((PCHAR) pCPUState->qwRBX);
}

// Code: 0x0002
// RBX is the x position
// RCX is the y position
void Syscall_KNeoGetCursorPosition(sCPUState *pCPUState)
{
    pCPUState->qwRBX = GetCursorX();
    pCPUState->qwRCX = GetCursorY();
}

// Code: 0x0003
// RBX is the x position
// RCX is the y position
void Syscall_KNeoSetCursorPosition(sCPUState *pCPUState)
{
    SetCursor(pCPUState->qwRBX, pCPUState->qwRCX);
}

// Code: 0x0004
void Syscall_KNeoClearScreen(sCPUState *pCPUState)
{
    ClearScreen();
}

// 0x0010 - 0x001F Driver and Module functions

// Code: 0x0010
// RBX is the address of the driver name
// RCX will then be the memory address of the allocated driver
void Syscall_KNeoGetDriver(sCPUState *pCPUState)
{
    
}

// Code: 0x0011
// RBX is the address of the module name
// RCX will then be the memory address of the allocated module
void Syscall_KNeoGetModule(sCPUState *pCPUState)
{
    
}

// Code: 0x0012
// RBX is the pointer to the driver
// RCX is the pointer to the function's name
// RDX will then contain the function
void Syscall_KNeoGetDriverFunction(sCPUState *pCPUState)
{
    
}

// Code: 0x0013
// RBX is the pointer to the module
// RCX is the pointer to the function's name
// RDX will then contain the function
void Syscall_KNeoGetModuleFunction(sCPUState *pCPUState)
{
    
}

// 0x0020 - 0x002F Memory functions

// Code: 0x0020
// RBX is the address of the pages
// RCX is the number of pages
// RDX contains the flags
void Syscall_KNeoMapPagesToIdentity(sCPUState *pCPUState)
{
    MapPageRangeToIdentity(NULL, (PVOID) pCPUState->qwRBX, pCPUState->qwRCX, pCPUState->qwRDX);
}

// Code: 0x0021
// EBX is the virtual address
// RCX is the number of pages
// RDX is the physical address
// ??? contains the flags
void Syscall_KNeoMapPages(sCPUState *pCPUState)
{
    MapPageRangeToIdentity(NULL, (PVOID) pCPUState->qwRBX, pCPUState->qwRCX, pCPUState->qwRDX);
}


// Code: 0x0022
// RCX contains the number of pages to allocate
// RBX will then be the address of the first page
void Syscall_KNeoAllocatePages(sCPUState *pCPUState)
{
    pCPUState->qwRBX = (QWORD) AllocateContinousPages(pCPUState->qwRCX);
}

// Code: 0x0023
// RBX is the address of the first page
// RCX contains the number of pages to free
void Syscall_KNeoFreePages(sCPUState *pCPUState)
{
    FreeContinousPages((PVOID) pCPUState->qwRBX, pCPUState->qwRCX);
}

// Code: 0x0024
// RCX is the size in bytes
// RBX will then contain the memory address
void Syscall_KNeoAllocateHeap(sCPUState *pCPUState)
{
    pCPUState->qwRBX = (QWORD) KHeapAlloc(pCPUState->qwRBX);
}

// Code: 0x0025
// RBX is the old memory address
// RCX is the new size in bytes
// RDX will then contain the re-allocated memory address
void Syscall_KNeoReAllocateHeap(sCPUState *pCPUState)
{
    pCPUState->qwRDX = (QWORD) KHeapReAlloc((PVOID) pCPUState->qwRBX, pCPUState->qwRCX);
}

// Code: 0x0026
// RBX contains the memory address
void Syscall_KNeoFreeHeap(sCPUState *pCPUState)
{
    KHeapFree((PVOID) pCPUState->qwRBX);
}

// 0x30 - 0x3F Other

// Code: 0x0030
// RBX is the Syscall code
// RCX is the ring level
// RDX is the callback
void Syscall_KNeoRegisterSyscall(sCPUState *pCPUState)
{
    RegisterSyscall(pCPUState->qwRBX, pCPUState->qwRCX, (void (*)(sCPUState *)) pCPUState->qwRDX);
}

// Code: 0x0031
// RDX is the callback
void Syscall_KNeoClearSyscall(sCPUState *pCPUState)
{
    ClearSyscall((void (*)(sCPUState *)) pCPUState->qwRDX);
}
