
#ifndef __GPT_H
#define __GPT_H

#include <common/types.h>

typedef struct
{
    BYTE  nBootIndicator;
    DWORD nCHSStart : 24;
    BYTE  nOSType;
    DWORD nCHSEnd : 24;
    DWORD dwLBAStart;
    DWORD dwLBAEnd;
    BYTE  arrReserved[494];
    WORD  wMagicNumber;
} __attribute__((packed)) sGPTProtectiveMasterBootRecord;

typedef struct
{
    QWORD qwSignature;
    DWORD dwRevision;
    DWORD dwHeaderSize;
    DWORD dwHeaderCRC32;
    DWORD dwReserved0;
    QWORD qwCurrentLBA;
    QWORD qwBackupLBA;
    QWORD qwFirstUsableLBA;
    QWORD qwLastUsableLBA;
    QWORD qwGUIDLow;
    QWORD qwGUIDHigh;
    QWORD qwStartLBA;
    DWORD dwNumPartitions;
    DWORD dwPartititionSize;
    DWORD dwPartititionEntryCRC32;
    BYTE  arrReserved1[420];
} __attribute__((packed)) sGPTPartitionTableHeader;

typedef struct
{
    BYTE arrPartitionTypeGUID[16];
    BYTE arrUniquePartitionGUID[16];
    QWORD qwLBAStart;
    QWORD qwLBAEnd;
    QWORD qwAttributes;
    BYTE  sPartitionName[72];
} __attribute__((packed)) sGPTPartitionEntry;

void LoadGPT(BYTE nDrive);
sGPTPartitionEntry GetKernelPartition();

#endif // __GPT_H
