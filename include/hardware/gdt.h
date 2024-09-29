
#ifndef __GDT_H
#define __GDT_H

#include <common/types.h>

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

void InitGDT(sGlobalDescriptorTable *pGDT);
extern void LoadGDT(sGlobalDescriptorTable *pGDT, uint16_t nSegments);

#endif // __GDT_H
