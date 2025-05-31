
#include <events/syscalls.h>
#include <runtime/process.h>
#include <neos.h>
#include <common/screen.h>
#include <hardware/idt.h>
#include <memory/list.h>

sList g_lstSyscallRegistry;

QWORD HandleSyscall(QWORD qwRSP)
{
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
}

