
#include <io/io.h>
#include <neos.h>
#include <memory/heap.h>
#include <common/string.h>
#include <archimports.h>

sObjectType *g_pDeviceObjectType,
            *g_pFileObjectType;

void DestroyDeviceObjectCallback(PVOID pObject)
{
    sDeviceObject *pDevice = pObject;
    KHeapFree(pDevice->ppDriverStack);
}

void DestroyFileObjectCallback(PVOID pObject)
{
    sFileObject *pFile = pObject;
    DestroyString(&pFile->wsPath);
}

INT DispatchIRP(sIORequestPacket *pIRP)
{
    if (pIRP->pDevice == NULL) return NEOS_NOT_FOUND | NEOS_ERROR_DEVICE;
    
    // sDriverObject *pDriver = *((sDriverObject **) GetListElement(&pIRP->pDevice->lstDriverStack, pIRP->qwDriverStackIndex++));
    sDriverObject *pDriver = pIRP->pDevice->ppDriverStack[pIRP->qwDriverStackIndex++];
    return DispatchDriverIRP(pDriver, pIRP);
}

STATUS CreateDevice(sDriverObject *pDriverObject, sString wsPath, DWORD dwDeviceType, sDeviceObject **ppDeviceObject)
{
    sObjectCreationParameters sParameters = MakeObjectCreationParameters(wsPath, NULL, 0);
    sDeviceObject *pObject;
    STATUS s = CreateObject(g_pDeviceObjectType, &sParameters, sizeof(sDeviceObject), (PVOID *) &pObject);
    if (!_SUCCESSFUL(s))
    {
        KHeapFree(pObject->ppDriverStack);
        return s;
    }

    pObject->ppDriverStack     = KHeapAlloc(sizeof(sDriverObject *));
    *pObject->ppDriverStack    = pDriverObject;
    pObject->dwDeviceType      = dwDeviceType;
    pObject->dwDriverStackSize = 1;

    if (pObject->ppDriverStack == NULL)
        return NEOS_ALLOCATION_FAILURE | NEOS_ERROR_IO;

    *ppDeviceObject = pObject;

    return NEOS_SUCCESS;
}

INT DestroyDevice(sDeviceObject *pDeviceObject)
{
    KHeapFree(pDeviceObject->ppDriverStack);
    KHeapFree(pDeviceObject);
    return NEOS_SUCCESS;
}


// If pParentDirectoryHandle is set to NULL, a full path must be specified
// If the specified path doesn't exist, a type must be specified using the wCreateType argument
INT CreateFile(sHandleTable *pCallersHandleTable, HANDLE *pHandle, sString wsPath,
               HANDLE hParentDirectory, WORD wAccessFlags, WORD wFileAttributes, WORD wShareFlags, WORD wCreateType)
{
    PVOID pParent = NULL;
    if (hParentDirectory != 0)
        pParent = GetObjectFromHandle(pCallersHandleTable, hParentDirectory);
    PVOID pObject = FindObject(wsPath.wszData, pParent);
    sObjectHeader *pHeader = (sObjectHeader *) pObject - 1;

    if (pObject != NULL && pHeader->pObjectType == g_pDeviceObjectType)
    {
        sFileObject *pFileObject;
        STATUS status = CreateObject(g_pFileObjectType, NULL, sizeof(sFileObject), (PVOID *) &pFileObject);

        if (!_SUCCESSFUL(status))
            return status;

        pFileObject->pDeviceObject = pObject;
        pFileObject->pContext      = NULL;
        pFileObject->wsPath        = CopyString(&wsPath);
        
        sIORequestPacket *pIRP = KHeapAlloc(sizeof(sIORequestPacket));
        pIRP->bOperation = IO_CREATE;
        pIRP->pBuffer = NULL;
        pIRP->qwLength = 0;
        pIRP->iStatus = 0;
        pIRP->pDevice = pObject;
        pIRP->pFileObject = pFileObject;
        pIRP->qwDriverStackIndex = 0;
        pIRP->pFinishCallback = NULL;

        INT iStatus = DispatchIRP(pIRP);
        KHeapFree(pIRP);

        if (_SUCCESSFUL(iStatus))
        {
            *pHandle = AllocateHandle(pCallersHandleTable, pFileObject);
            if (*pHandle == 0)
                return NEOS_ALLOCATION_FAILURE;
        }
        
        return iStatus;
    }

    
    // pObject = CreateObject(wszPath, g_pFileObjectType, );
    
    return NEOS_NOT_FOUND | NEOS_ERROR_OBJECT;
}

// If pParentDirectoryHandle is set to NULL, a full path must be specified
// If the specified path doesn't exist, a type must be specified using the wCreateType argument
INT ReadFile(sHandleTable *pCallersHandleTable, HANDLE hFile, PVOID pBuffer, QWORD qwLength)
{
    PVOID pObject = GetObjectFromHandle(pCallersHandleTable, hFile);
    if (pObject == NULL || ((sObjectHeader *) pObject - 1)->pObjectType != g_pFileObjectType)
        return NEOS_NOT_FOUND | NEOS_ERROR_OBJECT;

    sIORequestPacket *pIRP   = KHeapAlloc(sizeof(sIORequestPacket));

    pIRP->bOperation         = IO_READ;
    pIRP->pBuffer            = pBuffer;
    pIRP->qwLength           = qwLength;
    pIRP->iStatus            = 0;
    pIRP->pFileObject        = pObject;
    pIRP->pDevice            = ((sFileObject *) pObject)->pDeviceObject;
    pIRP->qwDriverStackIndex = 0;
    pIRP->pFinishCallback    = NULL;

    INT iStatus = DispatchIRP(pIRP);
    KHeapFree(pIRP);

    return iStatus;
}

INT CloseFile(sHandleTable *pCallersHandleTable, HANDLE hHandle)
{
    FreeHandle(pCallersHandleTable, hHandle);
    return NEOS_SUCCESS;
}

void InitIOManager()
{
    g_pDeviceObjectType = CreateType(MakeString(L"Device"), sizeof(sDeviceObject), DestroyDeviceObjectCallback, NULL);
    g_pFileObjectType   = CreateType(MakeString(L"FileObject"), sizeof(sFileObject), DestroyFileObjectCallback, NULL);
}
