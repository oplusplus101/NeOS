
#ifndef __PORTS_H
#define __PORTS_H


#include <common/types.h>

static inline BYTE inb(WORD wPort)
{
    BYTE nResult;
    __asm__ volatile ("in %%dx, %%al" : "=a"(nResult) : "d"(wPort));
    return nResult;
}

static inline WORD inw(WORD wPort)
{
    WORD wResult;
    __asm__ volatile ("in %%dx, %%ax" : "=a"(wResult) : "d"(wPort));
    return wResult;
}

static inline DWORD inl(WORD wPort)
{
    DWORD dwResult;
    __asm__ volatile ("in %%dx, %%eax" : "=a"(dwResult) : "d"(wPort));
    return dwResult;
}


static inline void outb(WORD wPort, BYTE nValue)
{
    __asm__ volatile ("out %%al, %%dx" :: "a"(nValue), "d"(wPort));
}

static inline void outw(WORD wPort, WORD wValue)
{
    __asm__ volatile ("out %%ax, %%dx" :: "a"(wValue), "Nd"(wPort));
}

static inline void outl(WORD wPort, DWORD dwValue)
{
    __asm__ volatile ("out %%eax, %%dx" :: "a"(dwValue), "Nd"(wPort));
}

static inline void insw(WORD wPort, PVOID pBuffer, QWORD nLength) {
    __asm__ volatile ("cld; rep; insw" : "+D"(pBuffer), "+c"(nLength) : "d"(wPort));
}

static inline void outsw(WORD wPort, PVOID pBuffer, QWORD nLength) {
    __asm__ volatile ("cld; rep; outsw" : "+S"(pBuffer), "+c"(nLength) : "d"(wPort));
}

static inline void IOWait()
{
    outb(0x80, 0);
}

#endif // __PORTS_H
