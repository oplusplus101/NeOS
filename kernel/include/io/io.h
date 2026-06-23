
#ifndef __IO__IO_H
#define __IO__IO_H

#include <common/types.h>
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
} sDeviceObject;

typedef struct
{
    sDeviceObject *pDeviceObject;
    sString wsPath;
    PVOID pContext;
} sFileObject;

typedef struct _tagIORequestPacket
{
    BYTE           bOperation;
    PVOID          pBuffer;
    QWORD          qwLength;
    INT            iStatus;
    sDeviceObject *pDevice;
    QWORD          qwDriverStackIndex;
    void (MS *pFinishCallback)(struct _tagIORequestPacket *);

    sFileObject   *pFileObject;
} sIORequestPacket;

INT DispatchIRP(sIORequestPacket *pIRP);
STATUS CreateDevice(sDriverObject *pDriverObject, sString wsPath, DWORD dwDeviceType, sDeviceObject **ppDeviceObject);
INT DestroyDevice(sDeviceObject *pDeviceObject);
INT CreateFile(sHandleTable *pCallersHandleTable, HANDLE *pHandle, sString wsPath,
               HANDLE hParentDirectory, WORD wAccessFlags, WORD wFileAttributes, WORD wShareFlags, WORD wCreateType);
INT ReadFile(sHandleTable *pCallersHandleTable, HANDLE hFile, PVOID pBuffer, QWORD qwLength);

void InitIOManager();

#endif // __IO__IO_H
