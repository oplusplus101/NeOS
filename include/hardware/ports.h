
#ifndef __PORTS_H
#define __PORTS_H


#include <common/types.h>

static inline uint8_t inb(uint16_t nPort)
{
    uint8_t nResult;
    __asm__ volatile ("in %%dx, %%al" : "=a"(nResult) : "d"(nPort));
    return nResult;
}

static inline uint16_t inw(uint16_t nPort)
{
    uint16_t nResult;
    __asm__ volatile ("in %%dx, %%ax" : "=a"(nResult) : "d"(nPort));
    return nResult;
}

static inline uint32_t inl(uint16_t nPort)
{
    uint32_t nResult;
    __asm__ volatile ("in %%dx, %%eax" : "=a"(nResult) : "d"(nPort));
    return nResult;
}


static inline void outb(uint16_t nPort, uint8_t nValue)
{
    __asm__ volatile ("out %%al, %%dx" :: "a"(nValue), "d"(nPort));
}

static inline void outw(uint16_t nPort, uint16_t nValue)
{
    __asm__ volatile ("out %%ax, %%dx" :: "a"(nValue), "Nd"(nPort));
}

static inline void outl(uint16_t nPort, uint32_t nValue)
{
    __asm__ volatile ("out %%eax, %%dx" :: "a"(nValue), "Nd"(nPort));
}

static inline void IOWait()
{
    outb(0x80, 0);
}

#endif // __PORTS_H
