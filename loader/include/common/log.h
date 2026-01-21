
#ifndef __COMMON__LOG_H
#define __COMMON__LOG_H

#include <common/types.h>

#define LOG_LOG 0
#define LOG_WARNING 1
#define LOG_ERROR 2
#define LOG_GOODBYE 3

void InitLogger();
extern void (*Log)(INT iType, const PWCHAR wszFormat, ...);

#endif // __LOG_H
