
#ifndef __HARDWARE__GDT_H
#define __HARDWARE__GDT_H

#include <common/types.h>

#define KERNEL_CODE_SEGMENT 0x10
#define KERNEL_DATA_SEGMENT 0x20
#define USER_CODE_SEGMENT   0x30
#define USER_DATA_SEGMENT   0x40
#define TASK_STATE_SEGMENT  0x50


typedef struct
{
    WORD  wLimitLow;
    DWORD nBaseLow    : 24;
    BYTE  nAccessByte;
    BYTE  nLimitHigh  : 4;
    BYTE  nFlags      : 4;
    QWORD nBaseHigh   : 40;
    DWORD dwReserved;
} __attribute__((packed)) sSegmentDescriptor;

typedef struct
{
    sSegmentDescriptor nullSegment;
    sSegmentDescriptor kernelCodeSegment;
    sSegmentDescriptor kernelDataSegment;
    sSegmentDescriptor userCodeSegment;
    sSegmentDescriptor userDataSegment;
    sSegmentDescriptor tssSegment;
} sGlobalDescriptorTable;

typedef struct
{
    DWORD dwReserved0;
    QWORD qwRSP0;
    QWORD qwRSP1;
    QWORD qwRSP2;
    QWORD qwReserved1;
    QWORD qwIST1;
    QWORD qwIST2;
    QWORD qwIST3;
    QWORD qwIST4;
    QWORD qwIST5;
    QWORD qwIST6;
    QWORD qwIST7;
    QWORD qwReserved2;
    WORD  wReserved3;
    WORD  wIOMapBaseAddress;
} __attribute__((packed)) sTaskStateSegment;


void MakeSegmentDescriptor(QWORD qwBase, DWORD nLimit, BYTE nAccessByte, BYTE nFlags, sSegmentDescriptor *pSegment);

sTaskStateSegment *GetTSS();
void InitGDT();
extern void WriteGDT();
extern void FlushTSS();

#endif // __HARDWARE__GDT_H
