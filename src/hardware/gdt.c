
#include <hardware/gdt.h>

void MakeSegmentDescriptor(uint64_t nBase, uint32_t nLimit, uint8_t nAccessByte, uint8_t nFlags, sSegmentDescriptor *pSegment)
{
    pSegment->nBaseLow    = nBase & 0xFFFFFF;
    pSegment->nBaseHigh   = nBase >> 24;
    pSegment->nLimitLow   = nLimit & 0xFFFF;
    pSegment->nLimitHigh  = nLimit >> 16;
    pSegment->nAccessByte = nAccessByte;
    pSegment->nFlags      = nFlags;
    pSegment->nReserved   = 0;
}

void InitGDT(sGlobalDescriptorTable *pGDT)
{
    MakeSegmentDescriptor(0, 0, 0, 0, &pGDT->nullSegment);
    MakeSegmentDescriptor(0, 0xFFFFFF, 0x9A, 0x0A, &pGDT->kernelCodeSegment);
    MakeSegmentDescriptor(0, 0xFFFFFF, 0x92, 0x0C, &pGDT->kernelDataSegment);
    MakeSegmentDescriptor(0, 0xFFFFFF, 0xFA, 0x0A, &pGDT->userCodeSegment);
    MakeSegmentDescriptor(0, 0xFFFFFF, 0xF2, 0x0C, &pGDT->userDataSegment);
}
