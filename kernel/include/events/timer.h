
#ifndef __EVENTS__TIMER_H
#define __EVENTS__TIMER_H

#include <common/types.h>

void SetPITFrequency(DWORD dwFrequency);
void SetPITInterval(DWORD dwIntervalMicroseconds);

#endif // __EVENTS__TIMER_H
