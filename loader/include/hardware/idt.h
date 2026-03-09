
#ifndef __HARDWARE__IDT_H
#define __HARDWARE__IDT_H

#include <common/types.h>

#define PIC_MASTER_COMMAND 0x20
#define PIC_SLAVE_COMMAND  0xA0

#define PIC_MASTER_DATA    0x21
#define PIC_SLAVE_DATA     0xA1

typedef struct
{
    WORD  wISRLow;
    WORD  wKernelCS;
    BYTE  bIST;
    BYTE  bFlags;
    WORD  wISRMid;
    DWORD dwISRHigh;
    DWORD dwReserved;
} __attribute__((packed)) sIDTEntry;
static_assert(sizeof(sIDTEntry) == 16, "sIDTEntry must be 16 bytes long");

typedef struct
{
    WORD  wLimit;
    QWORD qwBase;
} __attribute__((packed)) sIDTPointer;

typedef QWORD (FASTCALL *ISR)(QWORD);
typedef QWORD (FASTCALL *ESR)(QWORD, BYTE);

void InitIDT();
void RegisterException(BYTE n, ESR pESR);
void RegisterInterrupt(BYTE n, ISR pISR);

static inline void EnableInterrupts()
{
    __asm__ volatile ("sti");
}

static inline void DisableInterrupts()
{
    __asm__ volatile ("cli");
}

#endif // __HARDWARE__IDT_H
