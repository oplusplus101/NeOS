
#ifndef __IDT_H
#define __IDT_H

#include <common/types.h>

#define PIC_MASTER_COMMAND 0x20
#define PIC_SLAVE_COMMAND  0xA0

#define PIC_MASTER_DATA    0x21
#define PIC_SLAVE_DATA     0xA1

typedef struct
{
    WORD  wISRLow;
    WORD  wKernelCS;
    BYTE  nIST;
    BYTE  nFlags;
    QWORD nISRHigh : 48;
    DWORD dwReserved;
} __attribute__((packed)) sIDTEntry;

typedef struct
{
    WORD  wLimit;
    QWORD qwBase;
} __attribute__((packed)) sIDTPointer;

typedef QWORD (*ISR)(QWORD);
typedef QWORD (*ESR)(QWORD, BYTE);

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

#endif // __IDT_H
