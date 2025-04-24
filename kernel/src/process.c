
#include <process.h>
#include <hardware/gdt.h>
#include <memory/paging.h>
#include <memory/heap.h>
#include <common/screen.h>
#include <common/memory.h>
#include <common/string.h>

sList g_lstProcesses;
INT g_iCurrentPID;

void InitProcessScheduler()
{
    g_lstProcesses = CreateEmptyList(sizeof(sProcess));
    g_iCurrentPID = -1;
}

INT StartProcess(PCHAR szName, void (*pEntryPoint)(), QWORD qwStackSize, BYTE nRing, WORD wOwner)
{
    sProcess sProc;
    if (szName == NULL)
        *sProc.szName = 0;
    else
        strncpy(sProc.szName, szName, 128);
    sProc.nRing       = nRing;
    sProc.wOwner      = wOwner;
    sProc.iPID        = g_lstProcesses.qwLength;
    sProc.qwStackSize = qwStackSize;

    // Keep an unaligned stack so it can be freed
    sProc.pStackUnaligned = HeapAlloc(qwStackSize + 16);
    QWORD qwAlignOffset   = 16 - ((QWORD) sProc.pStackUnaligned & 0x0F);
    // Align the stack
    sProc.pStack          = (PBYTE) sProc.pStackUnaligned + qwAlignOffset;
    sProc.pCPUState       = (sCPUState *) ((QWORD) sProc.pStack + qwStackSize - sizeof(sCPUState));

    // Setup the CPU State
    sProc.pCPUState->qwRAX   = 0;
    sProc.pCPUState->qwRBX   = 0;
    sProc.pCPUState->qwRCX   = 0;
    sProc.pCPUState->qwRDX   = 0;

    sProc.pCPUState->qwR8    = 0;
    sProc.pCPUState->qwR9    = 0;
    sProc.pCPUState->qwR10   = 0;
    sProc.pCPUState->qwR11   = 0;
    sProc.pCPUState->qwR12   = 0;
    sProc.pCPUState->qwR13   = 0;
    sProc.pCPUState->qwR14   = 0;
    sProc.pCPUState->qwR15   = 0;

    sProc.pCPUState->qwRSI   = 0;
    sProc.pCPUState->qwRDI   = 0;
    sProc.pCPUState->qwRBP   = 0;

    sProc.pCPUState->qwRSP   = (QWORD) sProc.pStack + qwStackSize - sizeof(sCPUState);
    sProc.pCPUState->qwRIP   = (QWORD) pEntryPoint;
    sProc.pCPUState->qwCS    = KERNEL_CODE_SEGMENT;
    sProc.pCPUState->qwFlags = 0x0212; // Set the reserved and interrupt enable flags

    AddListElement(&g_lstProcesses, &sProc);
    return g_lstProcesses.qwLength - 1;
}

BOOL StopProcess(INT iPID)
{
    // TODO: Add a remove list element function and kill the process
    return false;
}

sProcess *GetProcess(INT iPID)
{
    if (iPID < 0) return NULL;
    return GetListElement(&g_lstProcesses, iPID);
}

QWORD ScheduleProcesses(QWORD qwRSP)
{
    if (g_lstProcesses.qwLength == 0) return qwRSP;

    if(g_iCurrentPID >= 0)
        ((sProcess *) GetListElement(&g_lstProcesses, g_iCurrentPID))->pCPUState = (sCPUState *) qwRSP;

    // If we've reached the last process, go back to the first.
    if (++g_iCurrentPID >= g_lstProcesses.qwLength)
        g_iCurrentPID = 0;
    
    return (QWORD) ((sProcess *) GetListElement(&g_lstProcesses, g_iCurrentPID))->pCPUState;
}
