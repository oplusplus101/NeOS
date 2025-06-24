
#include <runtime/process.h>
#include <runtime/exe.h>
#include <hardware/gdt.h>
#include <common/screen.h>
#include <common/memory.h>
#include <common/string.h>
#include <common/panic.h>
#include <memory/heap.h>
#include <events/timer.h>
#include <neos.h>

sList g_lstProcesses;
INT g_iCurrentPID;

void DummyProcess()
{
    for (;;)
        __asm__ volatile ("hlt");
}

void InitProcessScheduler(DWORD dwScheduleIntervalMicroseconds)
{
    g_lstProcesses = CreateEmptyList(sizeof(sProcess));
    g_iCurrentPID = -1;

    SetPITInterval(dwScheduleIntervalMicroseconds);
    
    // Setup the dummy process
    StartThread("Dummy", DummyProcess, 256, 0, 0);
    RegisterInterrupt(32, ScheduleProcesses);   
}

INT StartKernelProcess(PCHAR szName, sExecutable *pEXE, BYTE nState)
{
    QWORD qwStackSize = 0x100000; // 1MiB of stack space
    
    sProcess sProc;
    if (szName == NULL)
        *sProc.szName = 0;
    else
        strncpy(sProc.szName, szName, 128);
    sProc.nRing              = 0;
    sProc.nType              = 1;
    sProc.wOwner             = 0;
    sProc.iPID               = g_lstProcesses.qwLength;
    sProc.qwStackSize        = qwStackSize;
    sProc.nState             = nState;
    sProc.qwSleepTimeMS      = 0;

    // Keep an unaligned copy of the stack so it can be freed
    sProc.pStackUnaligned    = KHeapAlloc(qwStackSize + 16);
    _ASSERT(sProc.pStackUnaligned != NULL, "Could not allocate memory for executable: %s", szName);
    QWORD qwAlignOffset      = 16 - ((QWORD) sProc.pStackUnaligned & 0x0F);
    // Align the stack
    sProc.pStack             = (PBYTE) sProc.pStackUnaligned + qwAlignOffset;
    sProc.pCPUState          = (sCPUState *) ((QWORD) sProc.pStack + qwStackSize - sizeof(sCPUState));

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
    sProc.pCPUState->qwRIP   = (QWORD) pEXE->pEntryPoint;
    sProc.pCPUState->qwCS    = KERNEL_CODE_SEGMENT;
    sProc.pCPUState->qwFlags = 0x0202; // Set the reserved and interrupt enable flags

    // Setup paging
    sProc.pPML4 = ClonePML4(GetCurrentPML4());

    for (WORD i = 0; i < pEXE->lstSections.qwLength; i++)
    {
        sExecutableSection *pSection = GetListElement(&pEXE->lstSections, i);
        MapPageRange(sProc.pPML4, (PVOID) pSection->qwVirtualAddress, pSection->pData, _BYTES_TO_PAGES(pSection->dwVirtualSize), PF_WRITEABLE);
    }
    
    AddListElement(&g_lstProcesses, &sProc);
    return g_lstProcesses.qwLength - 1;
}

INT StartThread(PCHAR szName, void (*pEntryPoint)(), QWORD qwStackSize, BYTE nRing, WORD wOwner)
{
    _ASSERT(qwStackSize >= 256, "stack must be at least 256 bytes large");
    sProcess sProc;
    if (szName == NULL)
        *sProc.szName = 0;
    else
        strncpy(sProc.szName, szName, 128);

    sProc.nRing       = nRing;
    sProc.nType       = 0;
    sProc.wOwner      = wOwner;
    sProc.iPID        = g_lstProcesses.qwLength;
    sProc.qwStackSize = qwStackSize;

    // Keep an unaligned copy of the stack so it can be freed
    sProc.pStackUnaligned = KHeapAlloc(qwStackSize + 16);
    _ASSERT(sProc.pStackUnaligned != NULL, "Could not allocate memory for process: %s", szName);
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
    sProc.pCPUState->qwFlags = 0x0202; // Set the reserved and interrupt enable flags

    // Setup paging
    sProc.pPML4 = ClonePML4(GetCurrentPML4());

    AddListElement(&g_lstProcesses, &sProc);
    return g_lstProcesses.qwLength - 1;
}

void KillProcess(INT iPID, PWCHAR wszReason)
{
    SetFGColor(NEOS_ERROR_COLOR);
    PrintFormat("Process %s with PID %d was killed: %w\n", GetProcess(iPID)->szName, iPID, wszReason);
    SetFGColor(NEOS_FOREGROUND_COLOR);
    StopProcess(iPID);
}

BOOL StopProcess(INT iPID)
{
    if (!DoesProcessExist(iPID)) return false;
    SetProcessState(iPID, PROC_KILLED);
    return true;
}

sProcess *GetProcess(INT iPID)
{
    if (iPID < 0) return NULL;
    return GetListElement(&g_lstProcesses, iPID);
}

QWORD ScheduleProcesses(QWORD qwRSP)
{
    if (g_lstProcesses.qwLength == 0) return qwRSP;

    if  (g_iCurrentPID >= 0)
        GetProcess(g_iCurrentPID)->pCPUState = (sCPUState *) qwRSP;

    sProcess *pProcess = NULL;
    while (pProcess->nState != PROC_RUNNING || pProcess == NULL)
    {
        // If we've reached the last process, go back to the first.
        if (++g_iCurrentPID >= g_lstProcesses.qwLength)
            g_iCurrentPID = 0;
        pProcess = GetProcess(g_iCurrentPID);
        if (pProcess->nState == PROC_KILLED)
        {
            KHeapFree(pProcess->pStackUnaligned);
            FreePML4(pProcess->pPML4);
            RemoveListElement(&g_lstProcesses, g_iCurrentPID);
        }
    }
    
    LoadPML4(pProcess->pPML4);
    // PrintFormat("Scheduling %s RIP=%p 2RIP=%p\n", pProcess->szName, pProcess->pCPUState->qwRIP, ((sCPUState *) qwRSP)->qwRSP);
    return (QWORD) pProcess->pCPUState;
}

INT GetCurrentPID()
{
    return g_iCurrentPID;
}

sProcess *GetCurrentProcess()
{
    return GetListElement(&g_lstProcesses, g_iCurrentPID);
}

BOOL DoesProcessExist(INT iPID)
{
    for (QWORD i = 0; i < g_lstProcesses.qwLength; i++)
        if (GetProcess(i)->iPID == iPID)
            return true;
    
    return false;
}

BOOL SetProcessState(INT iPID, BYTE nState)
{
    if (!DoesProcessExist(iPID)) return false;
    ((sProcess *) GetListElement(&g_lstProcesses, iPID))->nState = nState;
    return true;
}
