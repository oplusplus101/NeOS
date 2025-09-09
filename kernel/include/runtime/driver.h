
#ifndef __RUNTIME__DRIVER_H
#define __RUNTIME__DRIVER_H

#include <common/types.h>
#include <runtime/process.h>
#include <runtime/exe.h>
#include <memory/list.h>

// Definition to avoid circular includes
struct _tagIORequestPacket;

typedef struct _tagDriverObject
{
    INT (*pDriverEntry)(struct _tagDriverObject *);
    INT (*pDriverUnload)(struct _tagDriverObject *);

    INT (*arrIOHandlers[5])(struct _tagDriverObject *, struct _tagIORequestPacket *);
    sPageTable *pPML4;
} __attribute__((packed)) sDriverObject;

typedef struct
{
    WCHAR wszName[129];
    sExecutable *pExecutable;
    sDriverObject *pDriverObject; // The object passed to the driver when the entry point is called.
} sDriver;


sDriver *GetDriver(PWCHAR wszName);
INT LoadDriver(sExecutable *pDriverExecutable, PWCHAR wszName);
INT DispatchDriverIRP(sDriverObject *pDriver, struct _tagIORequestPacket *pIRP);

void InitDriverManager();

#endif // __RUNTIME__DRIVER_H
