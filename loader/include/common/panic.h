
#ifndef __COMMON__PANIC_H
#define __COMMON__PANIC_H

#include <neos.h>

#define _KERNEL_PANIC(...) { Log(LOG_GOODBYE, __VA_ARGS__); __asm__ volatile ("cli\nhlt"); }

#define _ASSERT(c, ...) { if (!(c)) _KERNEL_PANIC(__VA_ARGS__); }

#endif // __COMMON__PANIC_H
