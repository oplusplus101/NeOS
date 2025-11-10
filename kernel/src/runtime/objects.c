
#include <runtime/objects.h>
#include <common/memory.h>
#include <memory/heap.h>
#include <common/string.h>
#include <loaderfunctions.h>


sObject *g_pRootDirectoryObject;
sObjectType *g_pDirectoryType;

////////////////////////////////
//        Object Stuff        //
////////////////////////////////

void DestroyDirectoryCallback(sObject *pObject)
{
    sObjectDirectory *pDirectory = pObject->pBody;

    for (QWORD i = 0; i < pDirectory->lstChildren.qwLength; i++)
    {
        sObject *pChild = *((sObject **) GetListElement(&pDirectory->lstChildren, i));

        if (pChild == NULL)
            continue;
        
        DestroyObjectByReference(pChild);
    }
    
    DestroyList(&pDirectory->lstChildren);
    KHeapFree(pDirectory);

    sObject *pParent = pObject->pParent;

    if (pParent == NULL && pParent->pType == g_pDirectoryType)
        return;

    sList *pParentList = &((sObjectDirectory *) pParent->pBody)->lstChildren;
    
    for (QWORD i = 0; i < pParentList->qwLength; i++)
    {
        sObject *pChild = *((sObject **) GetListElement(pParentList, i));

        if (pChild == pObject)
            RemoveListElement(pParentList, i);
    }
}

////////////////////////////////
//       Implementation       //
////////////////////////////////

BOOL AddObjectToDirectory(sObject *pDirectory, sObject *pObject)
{
    if (pDirectory->pType != g_pDirectoryType) return false;

    sObjectDirectory *pDirBody = (sObjectDirectory *) pDirectory->pBody;
    if (pDirBody == NULL) return false;

    AddListElement(&pDirBody->lstChildren, pObject);
    return true;
}

sObject *FindObjectInDirectory(sObjectDirectory *pDirectory, PWCHAR wszName)
{
    for (QWORD i = 0; i < pDirectory->lstChildren.qwLength; i++)
    {
        sObject *pObject = *((sObject **) GetListElement(&pDirectory->lstChildren, i));
        
        if (!strncmpW(pObject->wszName, wszName, 256))
            return pObject;
    }
    
    return NULL;
}


sObject *CreateObject(PWCHAR wszPath, sObjectType *pType, PVOID pBody)
{
    if (LookupObject(wszPath, NULL) != NULL) return NULL;
        
    PWCHAR wszPathDup         = strdupW(wszPath);
    sObject *pDirectoryObject = g_pRootDirectoryObject;
    PWCHAR wszCurrentName     = strtokW(wszPathDup, L"\\/");

    // FIXME: Add proper error handling in case a faulty path is entered
    while (wszCurrentName != NULL)
    {
        sObject *pObject = FindObjectInDirectory(pDirectoryObject->pBody, wszCurrentName);
        
        if (pObject == NULL)
            break;

        pDirectoryObject = pObject;
        wszCurrentName   = strtokW(NULL, L"\\/");

        if (wszCurrentName == NULL)
            break;

        if (pObject->pType == g_pDirectoryType)
            pDirectoryObject = pObject;
    }

    sObject *pObject          = KHeapAlloc(sizeof(sObject));
    pObject->dwReferenceCount = 1;
    pObject->pType            = pType;
    pObject->pBody            = KHeapAlloc(pType->qwBodySize);
    pObject->pParent          = pDirectoryObject;
    memcpy(pObject->pBody, pBody, pType->qwBodySize);
    strncpyW(pObject->wszName, wszCurrentName, 256);

    sObjectDirectory *pDirBody = (sObjectDirectory *) pDirectoryObject->pBody;
    if (pDirBody == NULL) return NULL;

    AddListElement(&pDirBody->lstChildren, &pObject);
    KHeapFree(wszPathDup);
    return pObject;
}

sObject *LookupObject(PWCHAR wszPath, sObject *pParent)
{
    if (pParent == NULL)
        pParent = g_pRootDirectoryObject;
    PWCHAR wszPathDup = strdupW(wszPath);
    PWCHAR wsz = strtokW(wszPathDup, L"\\/");
    
    while (wsz != NULL)
    {
        sObject *pObject = FindObjectInDirectory(pParent->pBody, wsz);
        if (pObject == NULL)
            break;
        
        wsz = strtokW(NULL, L"\\/");
        
        if (wsz == NULL)
        {
            KHeapFree(wszPathDup);
            return pObject;
        }
        if (pObject->pType == g_pDirectoryType)
            pParent = pObject;
    }
    
    KHeapFree(wszPathDup);
    return NULL;
}

sObject *CreateObjectDirectory(PWCHAR wszPath)
{
    sObjectDirectory sDirectory;
    sDirectory.lstChildren = CreateEmptyList(sizeof(sObject *));
    
    return CreateObject(wszPath, g_pDirectoryType, &sDirectory);
}

sObjectType *CreateType(PWCHAR wszName, QWORD qwBodySize, void (*pDestroyCallback)(sObject *))
{
    sObjectType *pObjectType      = KHeapAlloc(sizeof(sObjectType));
    pObjectType->pDestroyCallback = (void (*)(PVOID)) pDestroyCallback;
    pObjectType->qwBodySize       = qwBodySize;
    strncpyW(pObjectType->wszName, wszName, 256);
    return pObjectType;
}

void DestroyType(sObjectType *pType)
{
    KHeapFree(pType);
}

void DestroyObjectByReference(sObject *pObject)
{
    if (pObject == NULL) 
        return;
    
    if (pObject->pType->pDestroyCallback != NULL)
        pObject->pType->pDestroyCallback(pObject);

    // FIXME: Very dangerous code!!
    sList *pChildrenList = &((sObjectDirectory *) pObject->pParent->pBody)->lstChildren;
    for (QWORD i = 0; i < pChildrenList->qwLength; i++)
    {
        sObject *pChild = *((sObject **) GetListElement(pChildrenList, i));
        if (pChild == pObject)
            RemoveListElement(pChildrenList, i);
    }
    
    KHeapFree(pObject);
}

void DestroyObject(PWCHAR wszPath)
{
    DestroyObjectByReference(LookupObject(wszPath, NULL));
}

void ReferenceObject(sObject *pObject)
{
    ((sObject *) pObject)->dwReferenceCount++;
}

void DereferenceObject(sObject *pObject)
{
    // If the decremented value is 0, delete the object
    if (--pObject->dwReferenceCount == 0)
        DestroyObjectByReference(pObject);
}

////////////////////////////////
//        Handle Stuff        //
////////////////////////////////

sHandleTable CreateHandleTable()
{
    sHandleTable sTable =
    {
        .lstHandles            = CreateEmptyList(sizeof(sHandleEntry)),
        .qwCurrentHandleNumber = 1
    };
    
    return sTable;
}

void DestroyHandleTable(sHandleTable *pTable)
{
    DestroyList(&pTable->lstHandles);
}

HANDLE AllocateHandle(sHandleTable *pTable, sObject *pObject)
{
    sHandleEntry sEntry;
    sEntry.pObject = pObject;
    sEntry.hHandle = pTable->qwCurrentHandleNumber++;
    AddListElement(&pTable->lstHandles, &sEntry);
    return sEntry.hHandle;
}

sObject *GetObjectFromHandle(sHandleTable *pTable, HANDLE hHandle)
{
    for (QWORD i = 0; i < pTable->lstHandles.qwLength; i++)
    {
        sHandleEntry *pEntry = GetListElement(&pTable->lstHandles, i);
        if (pEntry->hHandle == hHandle)
            return pEntry->pObject;
    }
    
    return NULL;
}

void FreeHandle(sHandleTable *pTable, HANDLE hHandle)
{
    for (QWORD i = 0; i < pTable->lstHandles.qwLength; i++)
    {
        sHandleEntry *pEntry = GetListElement(&pTable->lstHandles, i);

        if (pEntry->hHandle != hHandle)
            continue;
        
        RemoveListElement(&pTable->lstHandles, i);
        if (pEntry->pObject != NULL)
            DereferenceObject(pEntry->pObject);
        return;
    }
}

void PrintObjectTree(sObject *pObject, DWORD dwCurrentDepth)
{
    if (pObject == NULL && dwCurrentDepth == 0)
        pObject = g_pRootDirectoryObject;
    if (dwCurrentDepth == 0)
        PrintFormat(L"%w\n", pObject->wszName);
    
    sObjectDirectory *pDirectory = pObject->pBody;
    
    for (QWORD i = 0; i < pDirectory->lstChildren.qwLength; i++)
    {
        sObject *pObject = *((sObject **) GetListElement(&pDirectory->lstChildren, i));

        PrintChar('\xB3'); // │
        SetCursor(dwCurrentDepth * 2, GetCursorY());

        if (i == pDirectory->lstChildren.qwLength - 1)
            PrintString("\xC0\xC4"); // └─
        else
            PrintString("\xC3\xC4"); // ├─
            
        PrintFormat(L"%w\n", pObject);

        if (pObject->pType == g_pDirectoryType)
            PrintObjectTree(pObject, dwCurrentDepth + 1);
    }
    
}

void InitObjectManager()
{
    // Initialise the types
    g_pDirectoryType = CreateType(L"Directory", sizeof(sObjectDirectory), DestroyDirectoryCallback);
    
    // Initialise the root object
    g_pRootDirectoryObject                   = KHeapAlloc(sizeof(sObject));
    g_pRootDirectoryObject->dwReferenceCount = 1;
    g_pRootDirectoryObject->pType            = g_pDirectoryType;
    g_pRootDirectoryObject->pParent          = NULL;
    g_pRootDirectoryObject->pBody            = KHeapAlloc(sizeof(sObjectDirectory));
    strncpyW(g_pRootDirectoryObject->wszName, L"<root>", 256);

    // Initialise the root directory
    sObjectDirectory *pRootDirectory         = g_pRootDirectoryObject->pBody;
    pRootDirectory->lstChildren              = CreateEmptyList(sizeof(sObject *));
}
