
#include <process.h>
#include <hardware/gdt.h>
#include <memory/paging.h>
#include <memory/heap.h>
#include <common/screen.h>
#include <common/memory.h>

sList g_lstProcesses;
INT g_iCurrentPID;

void InitProcessScheduler()
{
    g_lstProcesses = CreateEmptyList(sizeof(sProcess *));
    g_iCurrentPID = -1;
}

sProcess CreateProcess(void (*pEntryPoint)(), QWORD qwStackSize, BYTE nPermissionLevel, WORD wOwner)
{
    sProcess sProc;
    sProc.nPermissionLevel = nPermissionLevel;
    sProc.wOwner           = wOwner;
    // sProc.pStack           = AllocatePage();
    sProc.pStack           = HeapAlloc(qwStackSize);
    PrintFormat("ALDKR: %p\n", sProc.pStack);
    sProc.qwStackSize      = qwStackSize;
    sProc.pCPUState        = (sCPUState *) ((PBYTE) sProc.pStack + qwStackSize - sizeof(sCPUState));
    sProc.pCPUState->qwRAX = 0;
    sProc.pCPUState->qwRBX = 0;
    sProc.pCPUState->qwRCX = 0;
    sProc.pCPUState->qwRDX = 0;

    sProc.pCPUState->qwRSI = 0;
    sProc.pCPUState->qwRDI = 0;
    sProc.pCPUState->qwRBP = 0;
   
    sProc.pCPUState->qwRIP = (QWORD) pEntryPoint;
    sProc.pCPUState->qwCS  = KERNEL_CODE_SEGMENT; // TODO: Make segment change on the permission level (i.e. kernel and user mode)
    sProc.pCPUState->qwFlags = 0x202;

    return sProc;
}

DWORD StartProcess(sProcess *pProcess)
{
    AddListElement(&g_lstProcesses, pProcess);
    return g_lstProcesses.qwLength - 1;
}

BOOL StopProcess(DWORD dwPID)
{
    return false;
}

sCPUState *ScheduleProcesses(sCPUState *pCPUState)
{
    if (g_lstProcesses.qwLength == 0) return pCPUState;

    if(g_iCurrentPID >= 0)
        ((sProcess *) GetListElement(&g_lstProcesses, g_iCurrentPID))->pCPUState = pCPUState;

    if (++g_iCurrentPID >= g_lstProcesses.qwLength)
        g_iCurrentPID %= g_lstProcesses.qwLength;

    return ((sProcess *) GetListElement(&g_lstProcesses, g_iCurrentPID))->pCPUState;
}

