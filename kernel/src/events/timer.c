
#include <events/timer.h>
#include <hardware/ports.h>

void SetPITFrequency(DWORD dwFrequency)
{
    DWORD dwDivisor = 1193180 / dwFrequency;
    outb(0x43, 0x36);
    outb(0x40, dwDivisor & 0xFF);
    outb(0x40, dwDivisor >> 8);
}

void SetPITInterval(DWORD dwIntervalMicroseconds)
{
    // Some weird math to figure out the divisor
    DWORD dwDivisor = 59659 * dwIntervalMicroseconds / 50000;
    outb(0x43, 0x36);
    outb(0x40, dwDivisor & 0xFF);
    outb(0x40, dwDivisor >> 8);
}
