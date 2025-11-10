
#ifndef __IO__IO_H
#define __IO__IO_H

#include <common/types.h>
#include <common/list.h>
#include <runtime/driver.h>
#include <runtime/objects.h>

#define FILE_READ        1
#define FILE_WRITE       2
#define FILE_EXECUTE     4
#define FILE_SYNCRONISE  8
#define FILE_APPEND      16
#define FILE_DELETE      32
#define FILE_READ_ATTR   64
#define FILE_WRITE_ATTR  128

#define FILE_GENERIC_READ (FILE_SYNCRONISE | FILE_READ | FILE_READ_ATTR)
#define FILE_GENERIC_WRITE (FILE_SYNCRONISE | FILE_WRITE | FILE_WRITE_ATTR | FILE_APPEND)
#define FILE_GENERIC_EXECUTE (FILE_SYNCRONISE | FILE_READ_ATTR | FILE_EXECUTE)

enum
{
    IO_CREATE,
    IO_DESTROY,
    IO_READ,
    IO_WRITE,
    IO_CONTROL
};

typedef struct
{
    WCHAR wszName[256];
    DWORD dwDriverStackSize;
    sDriverObject **ppDriverStack;
    DWORD dwDeviceType;
} __attribute__((packed)) sDeviceObject;


typedef struct _tagIORequestPacket
{
    BYTE           nOperation;
    PVOID          pBuffer;
    QWORD          qwLength;
    INT            iStatus;
    sDeviceObject *pDevice;
    QWORD          qwDriverStackIndex;
    void         (*pFinishCallback)(struct _tagIORequestPacket *);
} __attribute__((packed)) sIORequestPacket;

INT DispatchIRP(sIORequestPacket *pIRP);
INT CreateDevice(sDriverObject *pDriverObject, PWCHAR wszDeviceName, DWORD dwDeviceType, sDeviceObject **ppDeviceObject);
INT DestroyDevice(sDeviceObject *pDeviceObject);
INT CreateFile(sHandleTable *pCallersHandleTable, HANDLE *pHandle, PWCHAR wszPath,
               HANDLE hParentDirectory, WORD wAccessFlags, WORD wFileAttributes, WORD wShareFlags, WORD wCreateType);

void InitIOManager();

#endif // __IO__IO_H
