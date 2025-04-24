
#ifndef __PROCESS_H
#define __PROCESS_H

#include <memory/list.h>
#include <hardware/idt.h>
#include <common/types.h>

typedef struct
{
    CHAR  szName[129]; // One extra byte for the null terminator
    BYTE  nRing;
    WORD  wOwner; // 0 is the kernel
    INT iPID;
    QWORD qwStackSize;
    PVOID pStackUnaligned, pStack;
    sCPUState *pCPUState;
} sProcess;

void InitProcessScheduler();

// Returns the PID
INT StartProcess(PCHAR szName, void (*pEntryPoint)(), QWORD qwStackSize, BYTE nRing, WORD wOwner);
BOOL StopProcess(INT iPID);
sProcess *GetProcess(INT iPID);

QWORD ScheduleProcesses(QWORD qwRSP);

#endif // __PROCESS_H
