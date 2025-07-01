
#ifndef __IO__TIMER_H
#define __IO__TIMER_H

#include <common/types.h>

void SetPITFrequency(DWORD dwFrequency);
void SetPITInterval(DWORD dwIntervalMicroseconds);

#endif // __IO__TIMER_H
