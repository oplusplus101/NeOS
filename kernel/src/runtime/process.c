
#include <runtime/process.h>
#include <runtime/exe.h>
#include <hardware/gdt.h>
#include <common/screen.h>
#include <common/memory.h>
#include <common/string.h>
#include <common/panic.h>
#include <memory/heap.h>
#include <io/timer.h>
#include <neos.h>

sList g_lstProcesses;
INT   g_iCurrentProcessIndex;

void DummyProcess()
{
    for (;;)
        __asm__ volatile ("hlt");
}

void InitProcessScheduler(DWORD dwScheduleIntervalMicroseconds)
{
    g_lstProcesses         = CreateEmptyList(sizeof(sProcess));
    g_iCurrentProcessIndex = -1;

    SetPITInterval(dwScheduleIntervalMicroseconds);
    
    // Setup the dummy process
    StartThread(L"Dummy", DummyProcess, 256, 0, 0, PROC_RUNNING);
    RegisterInterrupt(32, ScheduleProcesses);   
}

INT StartKernelProcess(PWCHAR wszName, sExecutable *pEXE, BYTE nState, PVOID pParams, QWORD qwParamSizeBytes)
{
    QWORD qwStackSize = 0x100000; // 1MiB of stack space
    
    sProcess sProc;
    if (wszName == NULL)
        *sProc.wszName = 0;
    else
        strncpyW(sProc.wszName, wszName, 128);
    sProc.nRing              = 0;
    sProc.nType              = 1;
    sProc.wOwner             = 0;
    sProc.iPID               = g_lstProcesses.qwLength;
    sProc.qwStackSize        = qwStackSize;
    sProc.nState             = nState;
    sProc.qwSleepTimeMS      = 0;

    // Keep an unaligned copy of the stack so it can be freed
    sProc.pStackUnaligned    = KHeapAlloc(qwStackSize + 16);
    _ASSERT(sProc.pStackUnaligned != NULL, L"Could not allocate memory for executable: %w", wszName);
    QWORD qwAlignOffset      = 16 - ((QWORD) sProc.pStackUnaligned & 0x0F);
    // Align the stack
    sProc.pStack             = (PBYTE) sProc.pStackUnaligned + qwAlignOffset;
    sProc.pCPUState          = (sCPUState *) ((QWORD) sProc.pStack + qwStackSize - sizeof(sCPUState) - qwParamSizeBytes);

    if (pParams != NULL && qwParamSizeBytes != 0)
        memcpy((PBYTE) sProc.pStack + qwStackSize - qwParamSizeBytes, pParams, qwParamSizeBytes);

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

    sProc.pCPUState->qwRSP   = (QWORD) sProc.pStack + qwStackSize - sizeof(sCPUState) - qwParamSizeBytes;
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
    return sProc.iPID;
}

INT StartThread(PWCHAR wszName, void (*pEntryPoint)(), QWORD qwStackSize, BYTE nRing, WORD wOwner, BYTE nState)
{
    _ASSERT(qwStackSize >= 256, L"Stack must be at least 256 bytes");
    sProcess sProc;
    if (wszName == NULL)
        *sProc.wszName = 0;
    else
        strncpyW(sProc.wszName, wszName, 128);

    sProc.nRing       = nRing;
    sProc.nType       = 0;
    sProc.wOwner      = wOwner;
    sProc.iPID        = g_lstProcesses.qwLength;
    sProc.qwStackSize = qwStackSize;
    sProc.nState      = nState;


    // Keep an unaligned copy of the stack so it can be freed
    sProc.pStackUnaligned = KHeapAlloc(qwStackSize + 16);
    _ASSERT(sProc.pStackUnaligned != NULL, L"Could not allocate memory for process: %w", wszName);
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
    return sProc.iPID;
}

void KillProcess(INT iPID, PWCHAR wszReason)
{
    if (!DoesProcessExist(iPID)) return;
    SetFGColor(NEOS_ERROR_COLOR);
    PrintFormat(L"Process %w with PID %d was killed: %w\n", GetProcess(iPID)->wszName, iPID, wszReason);
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
    for (QWORD i = 0; i < g_lstProcesses.qwLength; i++)
    {
        sProcess *pProcess = GetListElement(&g_lstProcesses, i);
            if (pProcess->iPID == iPID)
                return pProcess;
    }
    return NULL;
}

QWORD ScheduleProcesses(QWORD qwRSP)
{
    if (g_lstProcesses.qwLength == 0) return qwRSP;

    LoadPML4(GetKernelPML4());
    
    // Save the current process' state
    if  (g_iCurrentProcessIndex >= 0)
        ((sProcess *) GetListElement(&g_lstProcesses, g_iCurrentProcessIndex))->pCPUState = (sCPUState *) qwRSP;

    sProcess *pProcess = NULL;
    while (pProcess->nState != PROC_RUNNING || pProcess == NULL)
    {
        // If we've reached the last process, go back to the first.
        if (++g_iCurrentProcessIndex >= g_lstProcesses.qwLength)
            g_iCurrentProcessIndex = 0;

        pProcess = GetListElement(&g_lstProcesses, g_iCurrentProcessIndex);

        if (pProcess->nState == PROC_KILLED)
        {
            KHeapFree(pProcess->pStackUnaligned);
            FreePML4(pProcess->pPML4);
            PrintFormat(L"Killing %d\n", g_iCurrentProcessIndex);
            RemoveListElement(&g_lstProcesses, g_iCurrentProcessIndex);
        }
        
    }

    LoadPML4(pProcess->pPML4);
    return (QWORD) pProcess->pCPUState;
}

INT GetNumberOfOProcesses()
{
    return g_lstProcesses.qwLength;
}

INT GetCurrentPID()
{
    if (g_iCurrentProcessIndex == -1)
        return -1;
    return GetCurrentProcess()->iPID;
}

sProcess *GetCurrentProcess()
{
    return GetListElement(&g_lstProcesses, g_iCurrentProcessIndex);
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
    GetProcess(iPID)->nState = nState;
    return true;
}
