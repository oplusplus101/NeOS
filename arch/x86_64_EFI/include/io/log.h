
#ifndef __LOG_H
#define __LOG_H

#include <common/types.h>

#define LOG_LOG 0
#define LOG_WARNING 1
#define LOG_ERROR 2
#define LOG_GOODBYE 3

void SYSV ArchInternalLog(INT iType, const PWCHAR wszFormat, __builtin_va_list lst);
void MS ArchLog(INT iType, const PWCHAR wszFormat, ...);

#endif // __LOG_H
