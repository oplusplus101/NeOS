
#ifndef __MSR_H
#define __MSR_H


static inline QWORD rdmsr(DWORD dwMSR)
{
    DWORD dwValueLow, dwValueHigh;
    __asm__ volatile("rdmsr" : "=a"(dwValueLow), "=d"(dwValueHigh) : "c"(dwMSR));
    return dwValueLow | ((QWORD) dwValueHigh << 32);
}

static inline void wrmsr(DWORD dwMSR, QWORD qwValue)
{
    __asm__ volatile("wrmsr" : : "a"(qwValue & 0xFFFFFFFF), "d"(qwValue >> 32), "c"(dwMSR));
}

#endif // __MSR_H
