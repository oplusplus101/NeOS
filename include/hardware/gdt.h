
#ifndef __GDT_H
#define __GDT_H

#include <common/types.h>

#define KERNEL_CODE_SEGMENT 0x10
#define KERNEL_DATA_SEGMENT 0x20
#define USER_CODE_SEGMENT   0x30
#define USER_DATA_SEGMENT   0x40


typedef struct
{
    WORD  nLimitLow;
    DWORD nBaseLow : 24;
    BYTE  nAccessByte;
    BYTE  nLimitHigh : 4;
    BYTE  nFlags : 4;
    QWORD nBaseHigh : 40;
    DWORD nReserved;
} __attribute__((packed)) sSegmentDescriptor;

typedef struct
{
    sSegmentDescriptor nullSegment;
    sSegmentDescriptor kernelCodeSegment;
    sSegmentDescriptor kernelDataSegment;
    sSegmentDescriptor userCodeSegment;
    sSegmentDescriptor userDataSegment;
} sGlobalDescriptorTable;

void InitGDT();
extern void LoadGDT();

#endif // __GDT_H
