
#ifndef __RUNTIME__PROCESS_H
#define __RUNTIME__PROCESS_H

#include <memory/list.h>
#include <hardware/idt.h>
#include <common/types.h>
#include <memory/paging.h>
#include <runtime/exe.h>

#define PROC_THREAD 0
#define PROC_PROCESS 1

typedef struct
{
    CHAR  szName[129]; // One extra byte for the null terminator
    BYTE  nState; // 0 is running, 1 is sleeping, 2 is paused
    BYTE  nRing;
    WORD  wOwner; // 0 is the kernel
    INT   iPID;
    QWORD qwStackSize;
    QWORD qwSleepTimeMS;
    PVOID pStackUnaligned, pStack;
    sCPUState *pCPUState;
    sPageTable *pPML4;
} sProcess;

void InitProcessScheduler();

// Returns the PID
INT GetCurrentPID();
INT StartProcessExecutable(PCHAR szName, BYTE nRing, WORD wOwner, sExecutable *pEXE);
INT StartProcess(PCHAR szName, void (*pEntryPoint)(), QWORD qwStackSize, BYTE nRing, WORD wOwner);
void KillProcess(INT iPID, PWCHAR wszReason);
sProcess *GetProcess(INT iPID);
BOOL StopProcess(INT iPID);
BOOL DoesProcessExist(INT iPID);

QWORD ScheduleProcesses(QWORD qwRSP);

#endif // __RUNTIME__PROCESS_H
