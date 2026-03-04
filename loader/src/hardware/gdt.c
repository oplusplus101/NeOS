
#include <hardware/gdt.h>

sGlobalDescriptorTable g_sGDT __attribute__((aligned(16)));
sTaskStateSegment g_sTSS __attribute__((aligned(16)));

void MakeSegmentDescriptor(QWORD qwBase, DWORD dwLimit, BYTE bAccessByte, BYTE bFlags, sSegmentDescriptor *pDescriptor)
{
    pDescriptor->wBaseLow           = qwBase & 0xFFFF;
    pDescriptor->bBaseMid           = (qwBase >> 16) & 0xFF;
    pDescriptor->bBaseHigh          = qwBase >> 24;

    pDescriptor->wLimitLow          = dwLimit & 0xFFFF;
    pDescriptor->bLimitHighAndFlags = ((dwLimit >> 16) & 0x0F) | ((bFlags & 0x0F) << 4);
    
    pDescriptor->bAccessByte        = bAccessByte;
}

void MakeTSSDescriptor(QWORD qwBase, DWORD dwLimit, BYTE bAccessByte, BYTE bFlags, sTSSDescriptor *pDescriptor)
{
    pDescriptor->wBaseLow           = qwBase & 0xFFFF;
    pDescriptor->bBaseMid           = (qwBase >> 16) & 0xFF;
    pDescriptor->bBaseHigh          = qwBase >> 24;
    pDescriptor->dwBaseHighest      = qwBase >> 32;

    pDescriptor->wLimitLow          = dwLimit & 0xFFFF;
    pDescriptor->bLimitHighAndFlags = ((dwLimit >> 16) & 0x0F) | ((bFlags & 0x0F) << 4);

    pDescriptor->bAccessByte        = bAccessByte;
    pDescriptor->dwReserved         = 0;
}

sTaskStateSegment *GetTSS()
{
    return &g_sTSS;
}

void InitGDT()
{
    MakeSegmentDescriptor(0, 0, 0, 0,              &g_sGDT.sNullSegment);
    MakeSegmentDescriptor(0, 0xFFFFFF, 0x9A, 0x0A, &g_sGDT.sKernelCodeSegment);
    MakeSegmentDescriptor(0, 0xFFFFFF, 0x92, 0x0C, &g_sGDT.sKernelDataSegment);
    MakeSegmentDescriptor(0, 0xFFFFFF, 0xFA, 0x0A, &g_sGDT.sUserCodeSegment);
    MakeSegmentDescriptor(0, 0xFFFFFF, 0xF2, 0x0C, &g_sGDT.sUserDataSegment);
    
    MakeTSSDescriptor((QWORD) &g_sTSS, sizeof(sTaskStateSegment) - 1, 0x89, 0x00, &g_sGDT.sTSSSegment);
}
