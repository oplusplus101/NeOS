
#ifndef __GDT_H
#define __GDT_H

#include <common/types.h>

#define KERNEL_CODE_SEGMENT 0x10
#define KERNEL_DATA_SEGMENT 0x20
#define USER_CODE_SEGMENT   0x30
#define USER_DATA_SEGMENT   0x40


typedef struct
{
    uint16_t nLimitLow;
    uint32_t nBaseLow : 24;
    uint8_t  nAccessByte;
    uint8_t  nLimitHigh : 4;
    uint8_t  nFlags : 4;
    uint64_t nBaseHigh : 40;
    uint32_t nReserved;
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
