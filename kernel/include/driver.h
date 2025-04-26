
#ifndef __DRIVER_H
#define __DRIVER_H

#include <memory/list.h>

typedef struct
{
    PCHAR szName;
    PVOID pFunctionAddress;
} sDriverFunction; 

typedef struct
{
    PVOID pEntryPoint;
    sList lstExportedFunctions;
} sDriver;

sDriver LoadDriver(PCHAR szPath);

void UnloadDriver(sDriver *pDriver);

#endif // __DRIVER_H
