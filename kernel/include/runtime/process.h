
#ifndef __RUNTIME__PROCESS_H
#define __RUNTIME__PROCESS_H

#include <memory/list.h>
#include <hardware/idt.h>
#include <common/types.h>
#include <memory/paging.h>
#include <runtime/exe.h>

#define PROC_RUNNING  0
#define PROC_PAUSED   1
#define PROC_SLEEPING 2
#define PROC_KILLED   3

typedef struct
{
    WCHAR  wszName[129]; // One extra byte for the null terminator
    BYTE  nType;  // 0 is a thread, 1 is a process
    BYTE  nState; // 0 is running, 1 is sleeping, 2 is paused
    BYTE  nRing;
    WORD  wOwner; // 0 is the kernel
    INT   iPID;
    QWORD qwStackSize;
    QWORD qwSleepTimeMS;
    PVOID pStackUnaligned, pStack;
    sCPUState *pCPUState;
    sPageTable *pPML4;
    sExecutable pEXE;
} sProcess;

void InitProcessScheduler(DWORD dwScheduleIntervalMicroseconds);

// Returns the PID
INT GetCurrentPID();
INT StartKernelProcess(PWCHAR wszName, sExecutable *pEXE, BYTE nState, PVOID pParams, QWORD qwParamSizeBytes);
INT StartThread(PWCHAR wszName, void (*pEntryPoint)(), QWORD qwStackSize, BYTE nRing, WORD wOwner, BYTE nState);
void KillProcess(INT iPID, PWCHAR wszReason);
sProcess *GetCurrentProcess();
INT GetNumberOfOProcesses();
sProcess *GetProcess(INT iPID);
BOOL StopProcess(INT iPID);
BOOL DoesProcessExist(INT iPID);
BOOL SetProcessState(INT iPID, BYTE nState);

QWORD ScheduleProcesses(QWORD qwRSP);

#endif // __RUNTIME__PROCESS_H
