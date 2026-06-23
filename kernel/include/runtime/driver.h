
#ifndef __RUNTIME__DRIVER_H
#define __RUNTIME__DRIVER_H

#include <common/types.h>
#include <runtime/process.h>
#include <runtime/objects.h>
#include <runtime/exe.h>
#include <common/exestructs.h>

// Definition to avoid circular includes
struct _tagIORequestPacket;

typedef struct _tagDriverObject
{
    INT (MS *pDriverEntry)(struct _tagDriverObject *);
    INT (MS *pDriverUnload)(struct _tagDriverObject *);

    INT (MS *arrIOHandlers[5])(struct _tagDriverObject *, struct _tagIORequestPacket *);
} sDriverObject;

typedef struct
{
    WCHAR          wszName[129];
    sPE32Headers   sHeaders;
    QWORD          qwBaseAddress;
    sDriverObject *pDriverObject; // The object passed to the driver when the entry point is called.
} sDriver;


sDriver *GetDriver(PWCHAR wszName);
INT LoadDriver(PWCHAR wszName);
INT DispatchDriverIRP(sDriverObject *pDriver, struct _tagIORequestPacket *pIRP);

void InitDriverManager();

#endif // __RUNTIME__DRIVER_H
