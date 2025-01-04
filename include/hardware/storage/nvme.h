
#ifndef __NVME_H
#define __NVME_H

#include <common/types.h>

#define QUEUE_SIZE 63
#define NVME_SUCCESS 0

typedef struct
{
    PVOID pAddress;
    QWORD nSize;
} __attribute__((packed)) sNVMeQueue;

typedef struct
{
    BYTE nOpcode;
    DWORD nNamespaceID;
    PVOID *prp1;
    PVOID *prp2;
    DWORD arrCommandSpecific[3];
} __attribute__((packed)) sNVMeCommandEntry;

typedef struct
{
    DWORD nOpcode;
    DWORD nNamespaceID;
    DWORD arrReserved0;
    DWORD arrReserved1;
    DWORD nMetadataPointerLow;
    DWORD nMetadataPointerHigh;
    
} __attribute__((packed)) sNVMeSubmissionQueueEntry;

typedef struct
{
    DWORD nStatus;
} __attribute__((packed)) sNVMeCompletion;


BOOL CreateAdminSubmissionQueue(sNVMeQueue *pQueue);
BOOL CreateAdminCompletionQueue(sNVMeQueue *pQueue);

void SetupNVMe(BYTE nBus, BYTE nSlot, BYTE nFunction);

BOOL NVMeSendCommand(BYTE nOpcode, DWORD nNamespaceID, PVOID pData, QWORD nLBA, WORD num_blocks, sNVMeCompletion *pCompletion);
BOOL NVMeRead(QWORD nLBA, DWORD nSectors, PVOID pBuffer);
BOOL NVMeWrite(QWORD nLBA, DWORD nSectors, PVOID pBuffer);

#endif // __NVME_H
