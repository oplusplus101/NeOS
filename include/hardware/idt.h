
#ifndef __IDT_H
#define __IDT_H

#include <common/types.h>

#define PIC_MASTER_COMMAND 0x20
#define PIC_SLAVE_COMMAND  0xA0

#define PIC_MASTER_DATA    0x21
#define PIC_SLAVE_DATA     0xA1

typedef struct
{
    uint16_t nISRLow;
    uint16_t nKernelCS;
    uint8_t  nIST;
    uint8_t  nFlags;
    uint64_t nISRHigh : 48;
    uint32_t nReserved;
} __attribute__((packed)) sIDTEntry;

typedef struct
{
    uint16_t nLimit;
    uint64_t nBase;
} __attribute__((packed)) sIDTPointer;

typedef uint64_t (*ISR)(uint64_t);

void InitIDT();
void RegisterISR(uint8_t nInterrupt, ISR pISR);

static inline void EnableInterrupts()
{
    __asm__ volatile ("sti");
}

static inline void DisableInterrupts()
{
    __asm__ volatile ("cli");
}

#endif // __IDT_H
