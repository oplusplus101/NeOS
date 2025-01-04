
#include <hardware/storage/nvme.h>
#include <hardware/pci.h>
#include <hardware/memory/paging.h>
#include <common/memory.h>
#include <common/panic.h>

QWORD g_nNVMeBaseAddress;
QWORD g_nCompletionHead, g_nSubmissionQueueTail;
BYTE g_nNVMeCapabilityStride;

sNVMeQueue g_adminSubmissionQueue, g_adminCompletionQueue;

void SetupNVMe(BYTE nBus, BYTE nSlot, BYTE nFunction)
{
    sBaseAddressRegister bar = GetBaseAddressRegister(nBus, nSlot, nFunction, 0);
    g_nNVMeBaseAddress = bar.nAddress;
    g_nNVMeCapabilityStride = (g_nNVMeBaseAddress >> 12) & 0x0F;
    g_nCompletionHead = g_nSubmissionQueueTail = 0;

    MapPage((PVOID) g_nNVMeBaseAddress, (PVOID) g_nNVMeBaseAddress, PF_WRITEABLE);

    if (!CreateAdminSubmissionQueue(&g_adminSubmissionQueue))
        _KernelPanic("Failed to create an admin submission queue for NVMe");
    if (!CreateAdminCompletionQueue(&g_adminCompletionQueue))
        _KernelPanic("Failed to create an admin completion queue for NVMe");

    PrintFormat("NVMe Loaded! BUS=0x%02X, SLOT=0x%02X, FUNC=0x%02X, BAR: 0x%p\n", nBus, nSlot, nFunction, g_nNVMeBaseAddress);
}

QWORD NVMeReadRegister(QWORD nOffset)
{
    volatile QWORD *pNVMERegister = (volatile QWORD *)(g_nNVMeBaseAddress + nOffset);
    MapPage((PVOID *) pNVMERegister, (PVOID *) pNVMERegister, PF_WRITEABLE);
    return *pNVMERegister;
}

void NVMeWriteRegister(QWORD nOffset, QWORD nValue)
{
    volatile QWORD *pNVMERegister = (volatile QWORD *)(g_nNVMeBaseAddress + nOffset);
    MapPage((PVOID *) pNVMERegister, (PVOID *) pNVMERegister, PF_WRITEABLE);
    *pNVMERegister = nValue;
}

BOOL CreateAdminSubmissionQueue(sNVMeQueue *pQueue)
{
    pQueue->pAddress = AllocatePage();
    if (!pQueue->pAddress) return false;
    pQueue->nSize = QUEUE_SIZE;
    NVMeWriteRegister(0x28, (QWORD) pQueue->pAddress);
    return true;
}

BOOL CreateAdminCompletionQueue(sNVMeQueue *pQueue)
{
    pQueue->pAddress = AllocatePage();
    if (!pQueue->pAddress) return false;
    pQueue->nSize = QUEUE_SIZE;
    NVMeWriteRegister(0x30, (QWORD) pQueue->pAddress);
    return true;
}

BOOL NVMeSendCommand(BYTE nOpcode, DWORD nNamespaceID, PVOID pData, QWORD nLBA, WORD nBlocks, sNVMeCompletion *pCompletion)
{
    PVOID nSubmissionQueueEntry = (PVOID) ((QWORD) g_adminSubmissionQueue.pAddress + (g_nSubmissionQueueTail * sizeof(sNVMeCommandEntry)));
    PVOID nCompletionQueueEntry = (PVOID) ((QWORD) g_adminCompletionQueue.pAddress + (g_nCompletionHead * sizeof(sNVMeCompletion)));
    sNVMeCommandEntry command_entry;
    command_entry.nOpcode = nOpcode;
    command_entry.nNamespaceID = nNamespaceID;
    command_entry.prp1 = pData;
    command_entry.prp2 = 0;
    command_entry.arrCommandSpecific[0] = nLBA;
    command_entry.arrCommandSpecific[1] = nLBA >> 32;
    command_entry.arrCommandSpecific[2] = (WORD) (nBlocks - 1);
    memcpy(nSubmissionQueueEntry, &command_entry, sizeof(sNVMeCommandEntry));
    g_nSubmissionQueueTail++;
    NVMeWriteRegister(0x1000 + 2 * (4 << g_nNVMeCapabilityStride), g_nSubmissionQueueTail);
    if (g_nSubmissionQueueTail == QUEUE_SIZE)
        g_nSubmissionQueueTail = 0;
    // You should wait for completion here
    MapPage(nCompletionQueueEntry, nCompletionQueueEntry, PF_WRITEABLE);
    pCompletion = (sNVMeCompletion *) nCompletionQueueEntry;
    g_nCompletionHead++;
    NVMeWriteRegister(0x1000 + 3 * (4 << g_nNVMeCapabilityStride), g_nCompletionHead);
    if (g_nCompletionHead == QUEUE_SIZE)
        g_nCompletionHead = 0;
    return pCompletion->nStatus != 0;
}

BOOL NVMeRead(QWORD nLBA, DWORD nSectors, PVOID pBuffer)
{
    sNVMeCompletion *pCompletion = NULL;
    int nNamespaceID = 0;
    if (NVMeSendCommand(0x02, nNamespaceID, pBuffer, nLBA, nSectors, pCompletion) != NVME_SUCCESS)
        return false;
    if (pCompletion->nStatus != NVME_SUCCESS)
        return false;
    return true;
}

BOOL NVMeWrite(QWORD nLBA, DWORD nSectors, PVOID pBuffer)
{
    sNVMeCompletion *pCompletion = NULL;
    int nNamespaceID = 0;
    if (NVMeSendCommand(0x01, nNamespaceID, pBuffer, nLBA, nSectors, pCompletion) != NVME_SUCCESS)
        return false;
    if (pCompletion->nStatus != NVME_SUCCESS)
        return false;
    return true;
}
