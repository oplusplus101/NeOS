
#ifndef __LOG_H
#define __LOG_H

#include <common/screen.h>

#define LOG_LOG 0
#define LOG_WARNING 1
#define LOG_ERROR 2
#define LOG_GOODBYE 3

void InitLogger();
void Log(int iType, const PWCHAR wszFormat, ...);

#endif // __LOG_H
