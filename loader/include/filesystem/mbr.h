
#ifndef __MBR_H
#define __MBR_H

#include <common/types.h>

typedef struct
{
    BYTE  nDriveAttributes;
    DWORD nPartitionStartCHS : 24;
    BYTE  nPartitionType;
    DWORD nPartitionEndCHS : 24;
    DWORD nPartitionStartLBA;
    DWORD nPartitionSectorSize;
} __attribute__((packed)) sMBRPartitionEntry;

typedef struct
{
    BYTE arrBootloader[440];
    DWORD dwSignature;
    WORD dwReserved;
    sMBRPartitionEntry arrPrimaryPartitions[4];
    WORD dwMagicNumber;
} __attribute__((packed)) sMBRMasterBootRecord;

#endif // __MBR_H
