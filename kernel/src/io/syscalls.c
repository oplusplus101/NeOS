
#include <io/syscalls.h>
#include <runtime/process.h>
#include <neos.h>
#include <common/screen.h>
#include <hardware/idt.h>
#include <common/list.h>

sList g_lstSyscallRegistry;

extern void SetupSysInstructions();

QWORD HandleSyscall(QWORD qwRSP)
{
    // // Backup the PML4
    // sPageTable *pPML4 = GetCurrentPML4();
    // LoadPML4(GetKernelPML4());
    
    sCPUState *pCPUState = (sCPUState *) qwRSP;

    for (QWORD i = 0; i < g_lstSyscallRegistry.qwLength; i++)
    {
        sSyscallRegistryEntry *pEntry = GetListElement(&g_lstSyscallRegistry, i);
        if (pEntry->qwCode == pCPUState->qwRAX)
        {
            pEntry->pCallback(pCPUState);
            break;
        }
    }
    
    // // Restore the previous PML4
    // LoadPML4(pPML4);
    return qwRSP;
}

void RegisterSyscall(QWORD qwCode, BYTE nRing, void (*pCallback)(sCPUState *))
{
    sSyscallRegistryEntry sEntry;
    sEntry.qwCode = qwCode;
    sEntry.nRing = nRing;
    sEntry.pCallback = pCallback;
    AddListElement(&g_lstSyscallRegistry, &sEntry);
}

void ClearSyscall(void (*pCallback)(sCPUState *))
{
    for (QWORD i = 0; i < g_lstSyscallRegistry.qwLength; i++)
    {
        sSyscallRegistryEntry *pEntry = GetListElement(&g_lstSyscallRegistry, i);
        if (pEntry->pCallback == pCallback)
            RemoveListElement(&g_lstSyscallRegistry, i);
    }
}

void InitSyscalls()
{
    RegisterInterrupt(NEOS_SYSCALL_IRQ, HandleSyscall);
    g_lstSyscallRegistry = CreateEmptyList(sizeof(sSyscallRegistryEntry));
    SetupSysInstructions();
}
