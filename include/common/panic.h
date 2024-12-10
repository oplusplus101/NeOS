
#ifndef __PANIC_H
#define __PANIC_H

#include <common/screen.h>

#define _KernelPanic(...) { ClearScreen(); \
                            SetFGColor((color_t) { 255, 0, 0 }); \
                            PrintString("Kernel Panic!\nMessage: "); \
                            PrintFormat(__VA_ARGS__); \
                            __asm__ volatile ("cli\nhlt"); }
#define _KernelPanicEC(ec, ...) { ClearScreen(); \
                                  SetFGColor((color_t) { 255, 0, 0 }); \
                                  PrintString("Kernel Panic!\nMessage: "); \
                                  PrintFormat(__VA_ARGS__); \
                                  PrintFormat("\nError code: %d", ec); \
                                  __asm__ volatile ("cli\nhlt"); }

#endif // __PANIC_H
