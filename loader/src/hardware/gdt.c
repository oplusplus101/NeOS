
#include <hardware/gdt.h>

sGlobalDescriptorTable g_gdt;
sTaskStateSegment g_TSS;

void MakeSegmentDescriptor(QWORD qwBase, DWORD nLimit, BYTE nAccessByte, BYTE nFlags, sSegmentDescriptor *pSegment)
{
    pSegment->nBaseLow    = qwBase & 0xFFFFFF;
    pSegment->nBaseHigh   = qwBase >> 24;
    pSegment->wLimitLow   = nLimit & 0xFFFF;
    pSegment->nLimitHigh  = nLimit >> 16;
    pSegment->nAccessByte = nAccessByte;
    pSegment->nFlags      = nFlags;
    pSegment->dwReserved  = 0;
}

sTaskStateSegment *GetTSS()
{
    return &g_TSS;
}

void InitGDT()
{
    MakeSegmentDescriptor(0, 0, 0, 0, &g_gdt.nullSegment);
    MakeSegmentDescriptor(0, 0xFFFFFF, 0x9A, 0x0A, &g_gdt.kernelCodeSegment);
    MakeSegmentDescriptor(0, 0xFFFFFF, 0x92, 0x0C, &g_gdt.kernelDataSegment);
    MakeSegmentDescriptor(0, 0xFFFFFF, 0xFA, 0x0A, &g_gdt.userCodeSegment);
    MakeSegmentDescriptor(0, 0xFFFFFF, 0xF2, 0x0C, &g_gdt.userDataSegment);
    MakeSegmentDescriptor((QWORD) &g_TSS, sizeof(sTaskStateSegment), 0x89, 0x00, &g_gdt.tssSegment);
    // FlushTSS();
}
