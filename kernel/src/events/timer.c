
#include <events/timer.h>
#include <hardware/ports.h>

void SetTimerFrequency(DWORD dwFrequency)
{
    DWORD nDivisor = 1193180 / dwFrequency;

    outb(0x43, 0x36);
    outb(0x40, nDivisor & 0xFF);
    outb(0x40, nDivisor >> 8);
}
