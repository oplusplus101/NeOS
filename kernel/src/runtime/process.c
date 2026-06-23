
#include <runtime/process.h>
#include <runtime/exe.h>
#include <common/memory.h>
#include <common/string.h>
#include <common/panic.h>
#include <neos.h>
#include <archimports.h>

INT g_iCurrentPID = -1;
sObjectType *g_pProcessType;
sObjectDirectory *g_pProcessDirectory;
BOOL g_bScheduleingNeeded = false;

void DestroyProcess(PVOID pObject)
{
    sTask *pTask = pObject;
    if (pTask == NULL) return;

    RemoveFromLinkedList(&pTask->sListEntry);

    ArchFreePagesContinuous(pTask->pStack, _BYTES_TO_PAGES(pTask->qwStackSize));
    // if (pTask->bUser) 
        // FreePML4(pTask->pPML4);
}

void InitProcessScheduler(DWORD dwFrequency)
{
    // g_iCurrentPID = -1;
    // g_bScheduleingNeeded = false;

    // SetPITFrequency(dwFrequency);
    // SetSchedulerFunction(Schedule);

    // g_pProcessType = CreateType(MakeString(L"Process"), sizeof(sTask), DestroyProcess, NULL);
    // _ASSERT(g_pProcessType != NULL, L"Failed to create the Process type");

    // g_pProcessDirectory = FindObject(L"Processes", NULL);
    // _ASSERT(g_pProcessDirectory != NULL, L"Failed to find the Processes directory");
}

PVOID CreateUserPageTable()
{
    // PVOID pUserPML4 = ArchCreateUserPageTable();

    // // // Copy the kernel's pages
    // // sPageTable *pKernelPML4 = GetKernelPML4();
    // // for (WORD i = pKernelPML4->arrEntries[_ADDRESS_TO_PML4_INDEX(MM_KERNEL_REGION_START)]; pKernelPML4->arrEntries[i] != 0; i++)
    // //     pUserPML4->arrEntries[i] = pKernelPML4->arrEntries[i];
    
    // return pUserPML4;
    return NULL;
}

INT StartThread(PWCHAR wszName, void (*pEntryPoint)(), QWORD qwStackSize, BOOL bUser, WORD wOwner, BYTE bState)
{
    // _ASSERT(qwStackSize >= 256, L"Stack must be at least 256 bytes");

    // // Horrible way to do this, but it works
    // INT iPID = GetNewPID();
    // PWCHAR wszProcPath = L"           ";
    // itow(iPID, wszProcPath, 10);

    // sTask *pTask;
    // sObjectCreationParameters sParams = MakeObjectCreationParameters(MakeString(wszProcPath), g_pProcessDirectory, 0);
    // STATUS s = CreateObject(g_pProcessType, &sParams, sizeof(sTask), (PVOID *) &pTask);

    // if (!_SUCCESSFUL(s))
    //     _KERNEL_PANIC(L"Failed to start thread");
    
    // if (iPID == 0)
    //     InitLinkedList(&pTask->sListEntry);
    // else
    //     AppendIntoLinkedList(&GetProcess(0)->sListEntry, &pTask->sListEntry);

    // ZeroMemory(pTask->wszName, sizeof(pTask->wszName));
    // if (wszName != NULL)
    //     strncpyW(pTask->wszName, wszName, 128);

    // pTask->bUser         = bUser;
    // pTask->wOwner        = wOwner;
    // pTask->iPID          = iPID;
    // pTask->qwStackSize   = qwStackSize;
    // pTask->bState        = bState;
    // pTask->qwSleepTimeNS = 0;

    // pTask->pStack          = AllocateContinuousPages(_BYTES_TO_PAGES(qwStackSize));
    // pTask->pKernelStack    = AllocateContinuousPages(_BYTES_TO_PAGES(qwStackSize));
    // pTask->pKernelStackTop = (PBYTE) pTask->pKernelStack + qwStackSize;
    // _ASSERT(pTask->pStack != NULL, L"Could not allocate memory for process '%S' PID %d", pTask->wszName, iPID);

    // // Setup the CPU State
    // pTask->pCPUState          = (sCPUState *) ((QWORD) pTask->pStack + qwStackSize - sizeof(sCPUState));
    // pTask->pCPUState->qwRAX   = 0;
    // pTask->pCPUState->qwRBX   = 0;
    // pTask->pCPUState->qwRCX   = 0;
    // pTask->pCPUState->qwRDX   = 0;

    // pTask->pCPUState->qwR8    = 0;
    // pTask->pCPUState->qwR9    = 0;
    // pTask->pCPUState->qwR10   = 0;
    // pTask->pCPUState->qwR11   = 0;
    // pTask->pCPUState->qwR12   = 0;
    // pTask->pCPUState->qwR13   = 0;
    // pTask->pCPUState->qwR14   = 0;
    // pTask->pCPUState->qwR15   = 0;

    // pTask->pCPUState->qwRSI   = 0;
    // pTask->pCPUState->qwRDI   = 0;
    // pTask->pCPUState->qwRBP   = 0;

    // pTask->pCPUState->qwRIP   = (QWORD) pEntryPoint;
    // pTask->pCPUState->qwCS    = bUser ? USER_CODE_SEGMENT : KERNEL_CODE_SEGMENT;
    // pTask->pCPUState->qwFlags = 0x0202;
    
    // pTask->pCPUState->qwRSP   = (QWORD) pTask->pStack + qwStackSize - sizeof(sCPUState);
    // pTask->pCPUState->qwSS    = bUser ? USER_DATA_SEGMENT : KERNEL_DATA_SEGMENT;

    // // Setup paging
    // pTask->pPageTable = bUser ? CreateUserPageTable() : GetKernelPML4();
    // ArchMapPages(pTask->pPageTable, pTask->pStack, _BYTES_TO_PAGES(qwStackSize), PAGING_WRITEABLE | (bUser ? PAGING_WRITEABLE : 0));
    // ArchMapPages(pTask->pPageTable, pTask->pKernelStack, _BYTES_TO_PAGES(qwStackSize), PAGING_WRITEABLE);

    // // TEMPORARY!

    // if (bUser)
    //     ArchMapPages(pTask->pPageTable, (PVOID) (QWORD) pEntryPoint, 1, PAGING_WRITEABLE | PAGING_WRITEABLE);

    // return pTask->iPID;
    return 0;
}

BOOL KillProcess(INT iPID)
{
    return SetProcessState(iPID, PROC_KILLED);
}

sTask *GetProcess(INT iPID)
{
    if (iPID == -1)
        return NULL;
    PWCHAR wszProcPath = L"           ";
    itow(iPID, wszProcPath, 10);
    return FindObject(wszProcPath, g_pProcessDirectory);
}

sTask *GetNextProcess(INT iPID)
{
    if (g_iCurrentPID == -1)
        return GetProcess(0);
    
    return _GET_LINKED_LIST_DATA(GetCurrentProcess()->sListEntry.pForwards, sTask, sListEntry);
}

void PrintRegs(QWORD qwRSP);

QWORD Schedule(QWORD qwRSP)
{
    // PrintRegs(qwRSP);
    // if (GetNumberOfOProcesses() == 0) return qwRSP;

    // // Save the current process' state
    // if (g_iCurrentPID != -1)
    //     GetCurrentProcess()->pCPUState = (sCPUState *) qwRSP;

    // sTask *pTask = NULL;
    // INT iPID = g_iCurrentPID;
    // while (pTask == NULL || pTask->bState != PROC_RUNNING)
    // {
    //     sTask *pProcessObject = GetNextProcess(iPID);
    //     // Probably a bad idea, as this function gets called inside an interrupt
    //     _ASSERT(pProcessObject != NULL, L"Couldn't get the next process");
    //     pTask = pProcessObject;
    //     iPID = pTask->iPID;

    //     if (pTask->bState == PROC_KILLED)
    //     {
    //         DestroyObjectByReference(pProcessObject);
    //         pTask = NULL;
    //         continue;
    //     }
    // }

    
    // g_iCurrentPID = iPID;

    // ArchSetKernelStackTop((QWORD) pTask->pKernelStackTop);
    // ArchLoadPageTable(pTask->pPageTable);

    // return (QWORD) pTask->pCPUState;
    return 0;
}

DWORD GetNumberOfOProcesses()
{
    return g_pProcessDirectory->dwNumEntries;
}

INT GetNewPID()
{
    INT iPID = 0;

    while (DoesProcessExist(iPID))
        iPID++;
    
    return iPID;
}

INT GetCurrentPID()
{
    return g_iCurrentPID;
}

sTask *GetCurrentProcess()
{
    return GetProcess(g_iCurrentPID);
}

BOOL DoesProcessExist(INT iPID)
{
    return GetProcess(iPID) != NULL;
}

BOOL SetProcessState(INT iPID, BYTE bState)
{
    sTask *pTask = GetProcess(iPID);
    if (pTask == NULL)
        return false;
    pTask->bState = bState;
    return true;
}
