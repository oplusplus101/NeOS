
#ifndef __PORTS_H
#define __PORTS_H


#include <common/types.h>

static inline BYTE inb(WORD nPort)
{
    BYTE nResult;
    __asm__ volatile ("in %%dx, %%al" : "=a"(nResult) : "d"(nPort));
    return nResult;
}

static inline WORD inw(WORD nPort)
{
    WORD nResult;
    __asm__ volatile ("in %%dx, %%ax" : "=a"(nResult) : "d"(nPort));
    return nResult;
}

static inline DWORD inl(WORD nPort)
{
    DWORD nResult;
    __asm__ volatile ("in %%dx, %%eax" : "=a"(nResult) : "d"(nPort));
    return nResult;
}


static inline void outb(WORD nPort, BYTE nValue)
{
    __asm__ volatile ("out %%al, %%dx" :: "a"(nValue), "d"(nPort));
}

static inline void outw(WORD nPort, WORD nValue)
{
    __asm__ volatile ("out %%ax, %%dx" :: "a"(nValue), "Nd"(nPort));
}

static inline void outl(WORD nPort, DWORD nValue)
{
    __asm__ volatile ("out %%eax, %%dx" :: "a"(nValue), "Nd"(nPort));
}

static inline void insw(WORD nPort, PVOID pBuffer, SIZE_T nLength) {
    __asm__ volatile ("cld; rep; insw" : "+D"(pBuffer), "+c"(nLength) : "d"(nPort));
}

static inline void outsw(WORD nPort, PVOID pBuffer, SIZE_T nLength) {
    __asm__ volatile ("cld; rep; outsw" : "+S"(pBuffer), "+c"(nLength) : "d"(nPort));
}

static inline void IOWait()
{
    outb(0x80, 0);
}

#endif // __PORTS_H
