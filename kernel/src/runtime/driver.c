
#include <runtime/driver.h>
#include <runtime/process.h>
#include <common/string.h>
#include <common/memory.h>
#include <io/io.h>
#include <neos.h>
#include <loaderfunctions.h>

sObjectType *g_pDriverObjectType;

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

sObject *GetDriver(PWCHAR wszName)
{
    WCHAR wszPath[256];
    strcpyW(wszPath, L"Drivers\\");
    strcatW(wszPath, wszName);
    return LookupObject(wszPath, NULL);
}

INT LoadDriver(sExecutable *pDriverExecutable, PWCHAR wszName)
{
    sDriverObject *pObject = KHeapAlloc(sizeof(sDriverObject));
    ZeroMemory(pObject, sizeof(sDriverObject));
    pObject->pDriverEntry = (INT (*)(struct _tagDriverObject *)) (QWORD) (pDriverExecutable->qwEntryPoint + pDriverExecutable->qwBaseAddress);


    Log(LOG_LOG, L"ENTRY %p", pObject->pDriverEntry);
    
    sPageTable *pCurrentPML4 = GetCurrentPML4();
    pObject->pPML4 = pDriverExecutable->pPageTable;
    // Log(LOG_LOG, L"CURR: %p\n", PML4Hash(pCurrentPML4));
    // Log(LOG_LOG, L"POBJ: %p\n", PML4Hash(pObject->pPML4));

    for (;;);
    // PML4
    for (WORD i = 0; i < PAGE_TABLE_SIZE; i++)
    {
        if (!(pObject->pPML4->arrEntries[i].wFlags & PF_PRESENT)) continue;
        sPageTable *pPageDirectory = (sPageTable *) _PAGE_TO_ADDRESS(pObject->pPML4->arrEntries[i].qwAddress);

        // Page Directory
        for (WORD j = 0; j < PAGE_TABLE_SIZE; j++)
        {
            if (!(pPageDirectory->arrEntries[j].wFlags & PF_PRESENT)) continue;
            sPageTable *pPageTable = (sPageTable *) _PAGE_TO_ADDRESS(pPageDirectory->arrEntries[j].qwAddress);

            // Page Table
            for (WORD k = 0; k < PAGE_TABLE_SIZE; k++)
            {
                if (!(pPageTable->arrEntries[k].wFlags & PF_PRESENT)) continue;
                sPageTable *pPageEntry = (sPageTable *) _PAGE_TO_ADDRESS(pPageTable->arrEntries[k].qwAddress);
                QWORD qVA = (QWORD) k * 4096 * 4096 +
                            (QWORD) j * 4096 * 4096 * 4096 +
                            (QWORD) i * 4096 * 4096 * 4096 * 4096;
                Log(LOG_LOG, L"VA %p PA %p", qVA, _PAGE_TO_ADDRESS(pPageEntry->arrEntries[0].qwAddress), GetPhysicalAddress(pObject->pPML4, (PVOID) qVA));
                for (QWORD l = 0; l < 0xffffff; l++) __asm__ volatile ("nop");
            }
        }
    }

    Log(LOG_LOG, L"Starting driver...");
    PrintFormat(L"CPT: %p\n", pObject->pPML4);
    LoadPML4(pObject->pPML4);
    INT iStatus = pObject->pDriverEntry(pObject);
    LoadPML4(pCurrentPML4);
    for (;;);
    
    sDriver sDrv =
    {
        .pDriverObject = pObject,
        .pExecutable   = pDriverExecutable
    };

    strcpyW(sDrv.wszName, wszName);
    
    WCHAR wszPath[256];
    strcpyW(wszPath, L"Drivers\\");
    strcatW(wszPath, wszName);
    CreateObject(wszPath, g_pDriverObjectType, &sDrv);

    return iStatus;
}

void DestroyDriver(sObject *pObject)
{

}

void InitDriverManager()
{
    g_pDriverObjectType = CreateType(L"Driver", sizeof(sDriver), DestroyDriver);
}
