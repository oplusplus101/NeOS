
#ifndef __RUNTIME__DRIVER_H
#define __RUNTIME__DRIVER_H

#include <common/types.h>
#include <memory/list.h>

typedef struct
{
    WCHAR wszName[129];
    INT iPID;
    sList lstExportedFunctions;
} sDriver;

void InitDrivers();
void AddDriver(sDriver sDrv);
sDriver *GetDriver(PWCHAR wszName);

#endif // __RUNTIME__DRIVER_H
