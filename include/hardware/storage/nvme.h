
#ifndef __NVME_H
#define __NVME_H

#include <common/types.h>

#define QUEUE_SIZE 63
#define NVME_SUCCESS 0

typedef struct
{
    PVOID pAddress;
    QWORD qwSize;
} __attribute__((packed)) sNVMeQueue;

typedef struct
{
    BYTE nOpcode;
    DWORD dwNamespaceID;
    PVOID *prp1;
    PVOID *prp2;
    DWORD arrCommandSpecific[3];
} __attribute__((packed)) sNVMeCommandEntry;

typedef struct
{
    DWORD dwOpcode;
    DWORD dwNamespaceID;
    DWORD arrReserved0;
    DWORD arrReserved1;
    DWORD dwMetadataPointerLow;
    DWORD dwMetadataPointerHigh;
    
} __attribute__((packed)) sNVMeSubmissionQueueEntry;

typedef struct
{
    DWORD dwStatus;
} __attribute__((packed)) sNVMeCompletion;


BOOL CreateAdminSubmissionQueue(sNVMeQueue *pQueue);
BOOL CreateAdminCompletionQueue(sNVMeQueue *pQueue);

void SetupNVMe(BYTE nBus, BYTE nSlot, BYTE nFunction);

BOOL NVMeSendCommand(BYTE nOpcode, DWORD dwNamespaceID, PVOID pData, QWORD qwLBA, WORD wum_blocks, sNVMeCompletion *pCompletion);
BOOL NVMeRead(QWORD qwLBA, DWORD dwSectors, PVOID pBuffer);
BOOL NVMeWrite(QWORD qwLBA, DWORD dwSectors, PVOID pBuffer);

#endif // __NVME_H
