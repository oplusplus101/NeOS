
#ifndef __PANIC_H
#define __PANIC_H

#include <common/screen.h>

#define _KERNEL_PANIC(...) { SetFGColor((color_t) { 255, 0, 0 }); \
                             PrintString("\nKERNEL PANIC!\nMessage: "); \
                             PrintFormat(__VA_ARGS__); \
                            __asm__ volatile ("cli\nhlt"); }

#define _KERNEL_PANIC_EC(ec, ...) { SetFGColor((color_t) { 255, 0, 0 }); \
                                    PrintString("\nKERNEL PANIC!\nMessage: "); \
                                    PrintFormat(__VA_ARGS__); \
                                    PrintFormat("\nError code: %d", ec); \
                                    __asm__ volatile ("cli\nhlt"); }

#define _ASSERT(c, ...) { if (!(c)) _KERNEL_PANIC(__VA_ARGS__); }

#endif // __PANIC_H
