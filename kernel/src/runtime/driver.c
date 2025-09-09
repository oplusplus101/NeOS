
#include <runtime/driver.h>
#include <runtime/process.h>
#include <common/string.h>
#include <common/memory.h>
#include <common/screen.h>
#include <io/io.h>
#include <neos.h>

sList g_lstDrivers;

INT DispatchDriverIRP(sDriverObject *pDriver, sIORequestPacket *pIRP)
{
    // FIXME: Remove magic number and replace it with a constant
    if (pIRP->nOperation >= 5 || pIRP->pDevice == NULL) return NEOS_FAILURE;
    INT (*pHandler)(struct _tagDriverObject *, sIORequestPacket *) = pDriver->arrIOHandlers[pIRP->nOperation];
    if (pHandler == NULL)
        return NEOS_FAILURE;
    
    sPageTable *pCurrentPML4 = GetCurrentPML4();
    LoadPML4(pDriver->pPML4);
    INT iStatus = pHandler(pDriver, pIRP);
    LoadPML4(pCurrentPML4);
    return iStatus;
}

sDriver *GetDriver(PWCHAR wszName)
{
    for (QWORD i = 0; i < g_lstDrivers.qwLength; i++)
    {
        sDriver *pDriver = GetListElement(&g_lstDrivers, i);
        if (!strcmpW(pDriver->wszName, wszName))
        return pDriver;
    }
    
    return NULL;
}

INT LoadDriver(sExecutable *pDriverExecutable, PWCHAR wszName)
{
    sDriverObject *pObject = KHeapAlloc(sizeof(sDriverObject));
    ZeroMemory(pObject, sizeof(sDriverObject));
    pObject->pDriverEntry = (INT (*)(struct _tagDriverObject *)) (QWORD) pDriverExecutable->pEntryPoint;

    sPageTable *pCurrentPML4 = GetCurrentPML4();
    sPageTable *pPML4Clone = ClonePML4(pCurrentPML4);

    for (WORD i = 0; i < pDriverExecutable->lstSections.qwLength; i++)
    {
        sExecutableSection *pSection = GetListElement(&pDriverExecutable->lstSections, i);
        if (pSection->szName[0] != '.' || pSection->qwVirtualAddress == 0)
            continue;

        if (pSection->qwVirtualAddress >= NEOS_KERNEL_LOC_LOW &&
            pSection->qwVirtualAddress <= NEOS_KERNEL_LOC_LOW) return NEOS_FAILURE;
        
        MapPageRange(pPML4Clone, (PVOID) pSection->qwVirtualAddress, pSection->pData, _BYTES_TO_PAGES(pSection->dwVirtualSize), PF_WRITEABLE);
    }
    
    pObject->pPML4 = pPML4Clone;
    
    LoadPML4(pPML4Clone);
    INT iStatus = pObject->pDriverEntry(pObject);
    LoadPML4(pCurrentPML4);
    
    sDriver sDrv =
    {
        .pDriverObject = pObject,
        .pExecutable   = pDriverExecutable
    };
    
    strcpyW(sDrv.wszName, wszName);

    AddListElement(&g_lstDrivers, &sDrv);
    
    return iStatus;
}

void InitDriverManager()
{
    g_lstDrivers = CreateEmptyList(sizeof(sDriver));
}
