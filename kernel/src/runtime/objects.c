
#include <runtime/objects.h>
#include <common/string.h>
#include <common/screen.h>
#include <memory/heap.h>

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

BOOL AddObjectToDirectory(sObject *pDirectory, sObject *pObject)
{
    if (pDirectory->pType != g_pDirectoryType) return false;
    AddListElement(&((sObjectDirectory *) pDirectory->pBody)->lstChildren, pObject);
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

sObject *LookupObject(PWCHAR wszPath)
{
    PWCHAR wszPathDup = strdupW(wszPath);
    PWCHAR wsz = strtokW(wszPathDup, L"\\/");
    sObject *pCurrentDirectoryObject = g_pRootDirectoryObject;
    
    while (wsz != NULL)
    {
        sObject *pObject = FindObjectInDirectory(pCurrentDirectoryObject->pBody, wsz);
        if (pObject == NULL)
        {
            KHeapFree(wszPathDup);
            return NULL;
        }
        
        wsz = strtokW(NULL, L"\\/");
        
        if (wsz == NULL)
        {
            KHeapFree(wszPathDup);
            return pObject;
        }
        if (pObject->pType == g_pDirectoryType)
            pCurrentDirectoryObject = pObject;
    }
    
    KHeapFree(wszPathDup);
    return NULL;
}

sObject *CreateObjectDirectory(PWCHAR wszPath)
{
    sObjectDirectory *pDirectory = KHeapAlloc(sizeof(sObjectDirectory));
    pDirectory->lstChildren      = CreateEmptyList(sizeof(sObject *));
    
    return CreateObject(wszPath, g_pDirectoryType, pDirectory);
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

sObject *CreateObject(PWCHAR wszPath, sObjectType *pType, PVOID pBody)
{
    if (LookupObject(wszPath) != NULL) return NULL;
        
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
    pObject->pBody            = pBody;
    pObject->pParent          = pDirectoryObject;
    strncpyW(pObject->wszName, wszCurrentName, 256);

    AddListElement(&((sObjectDirectory *) pDirectoryObject->pBody)->lstChildren, &pObject);
    KHeapFree(wszPathDup);
    return pObject;
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
    DestroyObjectByReference(LookupObject(wszPath));
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

INT AllocateHandle(sHandleTable *pTable, sObject *pObject)
{
    sHandleEntry sEntry;
    sEntry.pObject = pObject;
    sEntry.iHandle = pTable->qwCurrentHandleNumber++;
    AddListElement(&pTable->lstHandles, &sEntry);
    return sEntry.iHandle;
}

sObject *GetObjectFromHandle(sHandleTable *pTable, INT iHandle)
{
    for (QWORD i = 0; i < pTable->lstHandles.qwLength; i++)
    {
        sHandleEntry *pEntry = GetListElement(&pTable->lstHandles, i);
        if (pEntry->iHandle == iHandle)
            return pEntry->pObject;
    }
    
    return NULL;
}

void FreeHandle(sHandleTable *pTable, INT iHandle)
{
    for (QWORD i = 0; i < pTable->lstHandles.qwLength; i++)
    {
        sHandleEntry *pEntry = GetListElement(&pTable->lstHandles, i);

        if (pEntry->iHandle != iHandle)
            continue;
        
        RemoveListElement(&pTable->lstHandles, i);
        if (pEntry->pObject != NULL)
            DereferenceObject(pEntry->pObject);
        return;
    }
}

void PrintObjectTree(sObject *pObject, DWORD dwCurrentDepth)
{
    if (dwCurrentDepth == 0)
        PrintFormat("%w\n", pObject->wszName);
    
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
            
        PrintFormat("%w\n", pObject);

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
