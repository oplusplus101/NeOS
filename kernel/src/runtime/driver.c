
#include <runtime/driver.h>
#include <runtime/process.h>
#include <common/string.h>
#include <common/memory.h>
#include <common/panic.h>
#include <io/io.h>
#include <loaderfunctions.h>
#include <neos.h>

sObjectType *g_pDriverObjectType;
// FIXME: Temporary until proper driver allocation is implemented
QWORD g_qwDriverOffset = 0;

INT DispatchDriverIRP(sDriverObject *pDriver, sIORequestPacket *pIRP)
{
    // FIXME: Remove magic number and replace it with a constant
    if (pIRP->nOperation >= 5 || pIRP->pDevice == NULL) return NEOS_FAILURE;
    INT (*pHandler)(struct _tagDriverObject *, sIORequestPacket *) = pDriver->arrIOHandlers[pIRP->nOperation];
    if (pHandler == NULL)
        return NEOS_FAILURE;
    
    INT iStatus = pHandler(pDriver, pIRP);
    return iStatus;
}

sObject *GetDriver(PWCHAR wszName)
{
    WCHAR wszPath[256];
    strcpyW(wszPath, L"Drivers\\");
    strcatW(wszPath, wszName);
    return FindObject(wszPath, NULL);
}

INT LoadDriver(PWCHAR wszName)
{
    WCHAR wszPath[256];
    strcpyW(wszPath, L"NeOS\\Drivers\\");
    strcatW(wszPath, wszName);
    strcatW(wszPath, L".drv");
    PVOID pFile = LoaderOpenFile(wszPath);

    if (pFile == NULL)
    {
        Log(LOG_ERROR, L"Could not load file for driver '%S'", wszName);
        return NEOS_LOAD_FAILURE | NEOS_ERROR_DRIVER;        
    }

    sExecutable *pExecutable = KHeapAlloc(sizeof(sExecutable));
    INT iStatus = LoadExecutable(pFile, MM_DRIVERS_START + g_qwDriverOffset, GetKernelPML4(), pExecutable);
    g_qwDriverOffset += 0x10000000; // FIXME: Assume a 256MiB driver size for now
    
    if (!_SUCCESSFUL(iStatus))
    {
        Log(LOG_ERROR, L"Could not load executable for driver '%S'", wszName);
        return iStatus;
    }
    
    sDriverObject *pObject = KHeapAlloc(sizeof(sDriverObject));
    ZeroMemory(pObject, sizeof(sDriverObject));
    pObject->pDriverEntry = (INT (*)(struct _tagDriverObject *)) (QWORD) (pExecutable->qwEntryPoint + pExecutable->qwVirtualStart);

    Log(LOG_LOG, L"Starting driver '%S'...", wszName);
    iStatus = pObject->pDriverEntry(pObject);

    if (!_SUCCESSFUL(iStatus))
    {
        Log(LOG_WARNING, L"Driver freeing isn't implemented yet");
        return iStatus;
    }
    
    sDriver sDrv =
    {
        .pDriverObject = pObject,
        .pExecutable   = pExecutable
    };
    strcpyW(sDrv.wszName, wszName);

    memset(wszPath, 0, sizeof(wszPath));
    strcpyW(wszPath, L"Drivers\\");
    strcatW(wszPath, wszName);

    PrintFormat(L"address: %p\n", sDrv.pExecutable->qwVirtualStart);

    // Commented out temporarly, because the heap causes crashes
    // CreateObject(wszPath, g_pDriverObjectType, &sDrv);

    return iStatus;
}

void DestroyDriver(sObject *pObject)
{
    _KERNEL_PANIC(L"DestroyDriver isn't yet implemented");
}

void InitDriverManager()
{
    g_pDriverObjectType = CreateType(L"Driver", sizeof(sDriver), DestroyDriver);
}
