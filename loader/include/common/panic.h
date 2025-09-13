
#ifndef __COMMON__PANIC_H
#define __COMMON__PANIC_H

#include <common/screen.h>
#include <common/log.h>
#include <neos.h>

#define _KERNEL_PANIC(...) { SetFGColor(NEOS_ERROR_COLOR); \
                             PrintStringW(L"\nKERNEL PANIC!\nMessage: "); \
                             Log(LOG_GOODBYE, __VA_ARGS__); \
                            __asm__ volatile ("cli\nhlt"); }

#define _KERNEL_PANIC_EC(ec, ...) { SetFGColor(NEOS_ERROR_COLOR); \
                                    PrintStringW(L"\nKERNEL PANIC!\nMessage: "); \
                                    Log(LOG_GOODBYE, __VA_ARGS__); \
                                    PrintFormat(L"\nError code: %d", ec); \
                                    __asm__ volatile ("cli\nhlt"); }

#define _ASSERT(c, ...) { if (!(c)) _KERNEL_PANIC(__VA_ARGS__); }

#endif // __COMMON__PANIC_H
