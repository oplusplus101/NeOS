
#include <io/io.h>
#include <neos.h>
#include <memory/heap.h>
#include <common/string.h>
#include <common/screen.h>

sObjectType *g_pDeviceObjectType,
            *g_pFileObjectType;

void DestroyDeviceObjectCallback(sObject *pObject)
{
    sDeviceObject *pDevice = (sDeviceObject *) pObject->pBody;
    DestroyList(&pDevice->lstDriverStack);
    KHeapFree(pDevice);
}

INT DispatchIRP(sIORequestPacket *pIRP)
{
    if (pIRP->pDevice == NULL) return NEOS_FAILURE;
    
    sDriverObject *pDriver = *((sDriverObject **) GetListElement(&pIRP->pDevice->lstDriverStack, pIRP->qwDriverStackIndex++));
    return DispatchDriverIRP(pDriver, pIRP);
}

INT CreateDevice(sDriverObject *pDriverObject, PWCHAR wszPath, DWORD dwDeviceType, sDeviceObject **ppDeviceObject)
{
    sDeviceObject *pDeviceObject = KHeapAlloc(sizeof(sDeviceObject));
    if (pDeviceObject == NULL)
        return NEOS_FAILURE;

    pDeviceObject->lstDriverStack = CreateEmptyList(sizeof(sDriverObject *));
    pDeviceObject->dwDeviceType   = dwDeviceType;
    AddListElement(&pDeviceObject->lstDriverStack, &pDriverObject);

    sObject *pObject = CreateObject(wszPath, g_pDeviceObjectType, pDeviceObject);
    if (pObject == NULL)
    {
        DestroyList(&pDeviceObject->lstDriverStack);
        KHeapFree(pDeviceObject);
    }
    
    *ppDeviceObject = pDeviceObject;
    return NEOS_SUCCESS;
}

INT DestroyDevice(sDeviceObject *pDeviceObject)
{
    DestroyList(&pDeviceObject->lstDriverStack);
    KHeapFree(pDeviceObject);
    return NEOS_FAILURE;
}


extern sObject *g_pRootDirectoryObject;
// If pParentDirectoryHandle is set to NULL, a full path must be specified
// If the specified path doesn't exist, a type must be specified using the wCreateType argument
INT CreateFile(sHandleTable *pCallersHandleTable, HANDLE *pHandle, PWCHAR wszPath,
               HANDLE hParentDirectory, WORD wAccessFlags, WORD wFileAttributes, WORD wShareFlags, WORD wCreateType)
{
    sObject *pParent = NULL;
    if (hParentDirectory != 0)
        pParent = GetObjectFromHandle(pCallersHandleTable, hParentDirectory);
    sObject *pObject = LookupObject(wszPath, pParent);

    if (pObject != NULL && pObject->pType == g_pDeviceObjectType)
    {
        sIORequestPacket *sIRP = KHeapAlloc(sizeof(sIORequestPacket));
        sIRP->nOperation = IO_CREATE;
        sIRP->pBuffer = NULL;
        sIRP->qwLength = 0;
        sIRP->iStatus = 0;
        sIRP->pDevice = pObject->pBody;
        sIRP->qwDriverStackIndex = 0;
        sIRP->pFinishCallback = NULL;

        INT iStatus = DispatchIRP(sIRP);
        KHeapFree(sIRP);

        if (_NEOS_SUCCESS(iStatus))
        {
            *pHandle = AllocateHandle(pCallersHandleTable, pObject);
            if (*pHandle == 0)
                return NEOS_FAILURE;
        }
        
        return iStatus;
    }

    // pObject = CreateObject(wszPath, g_pFileObjectType, );
    
    return NEOS_FAILURE;
}

INT CloseFile(sHandleTable *pCallersHandleTable, HANDLE hHandle)
{
    FreeHandle(pCallersHandleTable, hHandle);
    return NEOS_SUCCESS;
}

void InitIOManager()
{
    g_pDeviceObjectType = CreateType(L"Device", sizeof(sDeviceObject), DestroyDeviceObjectCallback);
}
