
#ifndef __RUNTIME__PROCESS_H
#define __RUNTIME__PROCESS_H

#include <common/list.h>
#include <common/linkedlist.h>
#include <common/types.h>
#include <runtime/exe.h>
#include <memory/heap.h>

#define PROC_RUNNING  0
#define PROC_SLEEPING 1
#define PROC_KILLED   2

typedef struct
{
    sLinkedListEntry sListEntry;
    WCHAR  wszName[129]; // One extra byte for the null terminator
    BYTE  bState; // 0 is running, 1 is sleeping, 2 is paused
    BOOL  bUser;
    WORD  wOwner; // 0 is the kernel
    INT   iPID;
    QWORD qwStackSize;
    QWORD qwSleepTimeNS;
    PVOID pStack, pKernelStack, pKernelStackTop;
    sCPUState *pCPUState;
    PVOID pPML4;
} sTask;

typedef struct
{
    sTask *pFirstTask;
    BYTE   bSubsystem; // 0 is the NeOS subsystem
    sHeap *pHeap;
} sProcess;

void InitProcessScheduler(DWORD dwFrequency);

INT       GetNewPID();
INT       GetCurrentPID();
INT       StartThread(PWCHAR wszName, void (*pEntryPoint)(), QWORD qwStackSize, BOOL bUser, WORD wOwner, BYTE bState);
BOOL      KillProcess(INT iPID);
sTask    *GetCurrentProcess();
DWORD     GetNumberOfOProcesses();
sTask    *GetProcess(INT iPID);
BOOL      DoesProcessExist(INT iPID);
BOOL      SetProcessState(INT iPID, BYTE bState);

QWORD Schedule(QWORD qwRSP);

#endif // __RUNTIME__PROCESS_H
