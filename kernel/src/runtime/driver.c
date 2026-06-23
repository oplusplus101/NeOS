
#include <runtime/driver.h>
#include <runtime/process.h>
#include <common/string.h>
#include <common/memory.h>
#include <common/panic.h>
#include <io/io.h>
#include <archimports.h>
#include <neos.h>
#include <ramfile.h>

sObjectType *g_pDriverType, *g_pDriverObjectType;

sHeap g_sDriverHeap;

INT DispatchDriverIRP(sDriverObject *pDriver, sIORequestPacket *pIRP)
{
    // FIXME: Remove magic number and replace it with a constant
    if (pIRP->bOperation >= 5) return NEOS_INVALID_ARGUMENT;
    if (pIRP->pDevice == NULL) return NEOS_NOT_FOUND | NEOS_ERROR_DEVICE;
    INT (MS *pHandler)(struct _tagDriverObject *, sIORequestPacket *) = pDriver->arrIOHandlers[pIRP->bOperation];
    if (pHandler == NULL)
        return NEOS_NULL_POINTER | NEOS_ERROR_DEVICE;

    return pHandler(pDriver, pIRP);
}

sDriver *GetDriver(PWCHAR wszName)
{
    WCHAR wszPath[256];
    strcpyW(wszPath, L"Drivers\\");
    strcatW(wszPath, wszName);
    return FindObject(wszPath, NULL);
}

STATUS LoadDriver(PWCHAR wszName)
{
    WCHAR wszPath[256];
    strcpyW(wszPath, L"NeOS\\Drivers\\");
    strcatW(wszPath, wszName);
    strcatW(wszPath, L".drv");
    sRAMFile *pFile = GetRAMFile(wszPath);

    if (pFile == NULL)
    {
        ArchLog(LOG_ERROR, L"Could not load file for driver '%S'", wszName);
        return NEOS_LOAD_FAILURE | NEOS_ERROR_DRIVER;        
    }

    sPE32Headers sPEHeaders;
    STATUS status = ParsePEHeaders(pFile, &sPEHeaders);
    if (!_SUCCESSFUL(status))
        return status;
    
    // QWORD qwExecutableSize = CalculatePEVirtualSize(&sPEHeaders);
    QWORD qwExecutableSize = sPEHeaders.sPEOptional.dwSizeOfImage;
    _ASSERT(qwExecutableSize != 0, L"Driver executable has a size of 0");
    PVOID pMemory = HeapAlloc(&g_sDriverHeap, qwExecutableSize);

    // TODO: Proper error code
    _ASSERT(pMemory != NULL, L"Failed to allocate memory for drivers");

    ArchLog(LOG_LOG, L"Driver Start 0x%p  End 0x%p", pMemory, (QWORD) pMemory + qwExecutableSize);
    
    PVOID pEntryPoint;
    status = LoadDriverIntoMemory(pFile, &sPEHeaders, pMemory, &pEntryPoint);

    if (!_SUCCESSFUL(status))
    {
        ArchLog(LOG_ERROR, L"Could not load executable for driver '%S'", wszName);
        KHeapFree(sPEHeaders.arrSections); 
        HeapFree(&g_sDriverHeap, pMemory);
        return status;
    }

    sDriverObject *pObject = KHeapAlloc(sizeof(sDriverObject));
    ZeroMemory(pObject, sizeof(sDriverObject));
    pObject->pDriverEntry = (INT MS (*)(struct _tagDriverObject *))(QWORD) pEntryPoint;

    ArchLog(LOG_LOG, L"Driver size in memory: %u bytes", qwExecutableSize);
    CheckHeapIntegrity(&g_sDriverHeap);
    
    ArchLog(LOG_LOG, L"Starting driver '%S' at entry point 0x%p...", wszName, pEntryPoint);
    status = pObject->pDriverEntry(pObject);

    if (!_SUCCESSFUL(status))
    {
        ArchLog(LOG_WARNING, L"Driver freeing isn't implemented yet");
        return status;
    }
    
    memset(wszPath, 0, sizeof(wszPath));
    strcpyW(wszPath, L"Drivers\\");
    strcatW(wszPath, wszName);

    sObjectCreationParameters sParameters = MakeObjectCreationParameters(MakeString(wszPath), NULL, 0);

    sDriver *pDriver;
    status = CreateObject(g_pDriverType, &sParameters, sizeof(sDriver), (PVOID) &pDriver);
    if (_SUCCESSFUL(!status))
    {
        DestroyString(&sParameters.wsName);
        ArchLog(LOG_ERROR, L"Failed to create the driver's object");
        return status;
    }

    pDriver->pDriverObject = pObject,
    pDriver->qwBaseAddress = (QWORD) pMemory;
    memcpy(&pDriver->sHeaders, &sPEHeaders, sizeof(sPE32Headers));
    strcpyW(pDriver->wszName, wszName);

    return NEOS_SUCCESS;
}

void DestroyDriver(PVOID pDriver)
{
    _KERNEL_PANIC(L"DestroyDriver isn't yet implemented");
}

void DestroyDriverObject(PVOID pDriverObject)
{
    _KERNEL_PANIC(L"DestroyDriverObject isn't yet implemented");
}

void InitDriverManager()
{
    g_pDriverType       = CreateType(MakeString(L"Driver"), sizeof(sDriver), DestroyDriver, NULL);
    g_pDriverObjectType = CreateType(MakeString(L"DriverObject"), sizeof(sDriverObject), DestroyDriverObject, NULL);
    // 256 MiB
    if (!_SUCCESSFUL(CreateHeap(10L * 1024L * 1024L, false, true, true, true, PAGE_SIZE, (PVOID) MM_DRIVERS_START, &g_sDriverHeap)))
    // TODO: Return an error code
        _KERNEL_PANIC(L"Failed to create the driver heap");
}
