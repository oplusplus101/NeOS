
#include <hardware/storage/nvme.h>
#include <hardware/pci.h>
#include <memory/paging.h>
#include <common/memory.h>
#include <common/panic.h>

QWORD g_nNVMeBaseAddress;
QWORD g_nCompletionHead, g_nSubmissionQueueTail;
BYTE g_nNVMeCapabilityStride;

sNVMeQueue g_adminSubmissionQueue, g_adminCompletionQueue;

void SetupNVMe(BYTE nBus, BYTE nSlot, BYTE nFunction)
{
    sBaseAddressRegister bar = GetBaseAddressRegister(nBus, nSlot, nFunction, 0);
    g_nNVMeBaseAddress = bar.qwAddress;
    g_nNVMeCapabilityStride = (g_nNVMeBaseAddress >> 12) & 0x0F;
    g_nCompletionHead = g_nSubmissionQueueTail = 0;

    MapPageToIdentity(NULL, (PVOID) g_nNVMeBaseAddress, PF_WRITEABLE | PF_CACHEDISABLE);

    if (!CreateAdminSubmissionQueue(&g_adminSubmissionQueue))
        _KERNEL_PANIC(L"Failed to create an admin submission queue for NVMe");
    if (!CreateAdminCompletionQueue(&g_adminCompletionQueue))
        _KERNEL_PANIC(L"Failed to create an admin completion queue for NVMe");

    PrintFormat(L"NVMe Loaded! BUS=0x%02X, SLOT=0x%02X, FUNC=0x%02X, BAR: 0x%p\n", nBus, nSlot, nFunction, g_nNVMeBaseAddress);
}

QWORD qwVMeReadRegister(QWORD qwOffset)
{
    volatile QWORD *pNVMERegister = (volatile QWORD *)(g_nNVMeBaseAddress + qwOffset);
    MapPageToIdentity(NULL, (PVOID *) pNVMERegister, PF_WRITEABLE | PF_CACHEDISABLE);
    return *pNVMERegister;
}

void NVMeWriteRegister(QWORD qwOffset, QWORD qwValue)
{
    volatile QWORD *pNVMERegister = (volatile QWORD *)(g_nNVMeBaseAddress + qwOffset);
    MapPageToIdentity(NULL, (PVOID *) pNVMERegister, PF_WRITEABLE | PF_CACHEDISABLE);
    *pNVMERegister = qwValue;
}

BOOL CreateAdminSubmissionQueue(sNVMeQueue *pQueue)
{
    pQueue->pAddress = AllocatePage();
    if (!pQueue->pAddress) return false;
    pQueue->qwSize = QUEUE_SIZE;
    NVMeWriteRegister(0x28, (QWORD) pQueue->pAddress);
    return true;
}

BOOL CreateAdminCompletionQueue(sNVMeQueue *pQueue)
{
    pQueue->pAddress = AllocatePage();
    if (!pQueue->pAddress) return false;
    pQueue->qwSize = QUEUE_SIZE;
    NVMeWriteRegister(0x30, (QWORD) pQueue->pAddress);
    return true;
}

BOOL NVMeSendCommand(BYTE dwOpcode, DWORD dwNamespaceID, PVOID pData, QWORD qwLBA, WORD wBlocks, sNVMeCompletion *pCompletion)
{
    PVOID nSubmissionQueueEntry = (PVOID) ((QWORD) g_adminSubmissionQueue.pAddress + (g_nSubmissionQueueTail * sizeof(sNVMeCommandEntry)));
    PVOID nCompletionQueueEntry = (PVOID) ((QWORD) g_adminCompletionQueue.pAddress + (g_nCompletionHead * sizeof(sNVMeCompletion)));
    sNVMeCommandEntry command_entry;
    command_entry.nOpcode = dwOpcode;
    command_entry.dwNamespaceID = dwNamespaceID;
    command_entry.prp1 = pData;
    command_entry.prp2 = 0;
    command_entry.arrCommandSpecific[0] = qwLBA;
    command_entry.arrCommandSpecific[1] = qwLBA >> 32;
    command_entry.arrCommandSpecific[2] = (WORD) (wBlocks - 1);
    memcpy(nSubmissionQueueEntry, &command_entry, sizeof(sNVMeCommandEntry));
    g_nSubmissionQueueTail++;
    NVMeWriteRegister(0x1000 + 2 * (4 << g_nNVMeCapabilityStride), g_nSubmissionQueueTail);
    if (g_nSubmissionQueueTail == QUEUE_SIZE)
        g_nSubmissionQueueTail = 0;
    // You should wait for completion here
    MapPageToIdentity(NULL, nCompletionQueueEntry, PF_WRITEABLE | PF_CACHEDISABLE);
    pCompletion = (sNVMeCompletion *) nCompletionQueueEntry;
    g_nCompletionHead++;
    NVMeWriteRegister(0x1000 + 3 * (4 << g_nNVMeCapabilityStride), g_nCompletionHead);
    if (g_nCompletionHead == QUEUE_SIZE)
        g_nCompletionHead = 0;
    return pCompletion->dwStatus != 0;
}

BOOL NVMeRead(QWORD qwLBA, DWORD nSectors, PVOID pBuffer)
{
    sNVMeCompletion *pCompletion = NULL;
    int dwNamespaceID = 0;
    if (NVMeSendCommand(0x02, dwNamespaceID, pBuffer, qwLBA, nSectors, pCompletion) != NVME_SUCCESS)
        return false;
    if (pCompletion->dwStatus != NVME_SUCCESS)
        return false;
    return true;
}

BOOL NVMeWrite(QWORD qwLBA, DWORD nSectors, PVOID pBuffer)
{
    sNVMeCompletion *pCompletion = NULL;
    int dwNamespaceID = 0;
    if (NVMeSendCommand(0x01, dwNamespaceID, pBuffer, qwLBA, nSectors, pCompletion) != NVME_SUCCESS)
        return false;
    if (pCompletion->dwStatus != NVME_SUCCESS)
        return false;
    return true;
}
