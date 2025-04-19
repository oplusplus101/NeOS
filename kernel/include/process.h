
#ifndef __PROCESS_H
#define __PROCESS_H

#include <memory/list.h>
#include <hardware/idt.h>
#include <common/types.h>

typedef struct
{
    QWORD qwRAX;
    QWORD qwRBX;
    QWORD qwRCX;
    QWORD qwRDX;

    QWORD qwRSI;
    QWORD qwRDI;
    QWORD qwRBP;

    QWORD qwError;

    QWORD qwRIP;
    QWORD qwCS;
    QWORD qwFlags;
    QWORD qwRSP;
    QWORD qwSS;
} __attribute__((packed)) sCPUState;
 

typedef struct
{
    PVOID pStack;
    QWORD qwStackSize;
    sCPUState *pCPUState;
    BYTE nPermissionLevel;
    WORD wOwner; // 0 is the kernel
} sProcess;

void InitProcessScheduler();

sProcess CreateProcess(void (*pEntryPoint)(), QWORD qwStackSize, BYTE nPermissionLevel, WORD wOwner);

// Returns the PID
DWORD StartProcess(sProcess *pProcess);
BOOL  StopProcess(DWORD dwPID);

sCPUState *ScheduleProcesses(sCPUState *pCPUState);


#endif // __PROCESS_H
