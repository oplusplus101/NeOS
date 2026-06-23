
#include <runtime/objects.h>
#include <common/memory.h>
#include <memory/heap.h>
#include <common/string.h>
#include <common/panic.h>
#include <archimports.h>


sObjectDirectory *g_pRootDirectoryObject;
sObjectType *g_pObjectType;
sObjectType *g_pObjectDirectoryType;

sObjectDirectory *g_pTypeDirectory;

////////////////////////////////
//          Utilities         //
////////////////////////////////


/// @brief Calculates a FNV-1a hash
/// @param wszPath The string hash
/// @param bIgnoreCase Whether to ignore cases (only works with ASCII characters)
/// @return The hash result
QWORD HashPath(const PWCHAR wszPath, BOOL bIgnoreCase)
{
    QWORD qwHash = 0xcbf29ce484222325;
    
    for (QWORD i = 0; wszPath[i] != 0; i++)
    {
        WCHAR wc = wszPath[i];
        
        if (bIgnoreCase)
            qwHash ^= wc >= 'A' && wc <= 'Z' ? wc | 32 : (wc & 0xFF);
        qwHash *= 0x100000001b3;

        qwHash ^= wc >> 8;
        qwHash *= 0x100000001b3;
    }
    
    return qwHash;
}

void DestroyTypeCallback(PVOID pObject)
{
    sObjectType *pTypeObject = pObject;
    DestroyString(&pTypeObject->wsName);
}

////////////////////////////////
//      Directory Stuff       //
////////////////////////////////

void DestroyDirectoryCallback(PVOID pObject)
{
    sObjectDirectory *pDirectory = pObject;

    sObjectEnumerator sEnum = CreateObjectDirectoryEnumerator(pDirectory);
    for (PVOID pObject = GetEnumeratedObject(&sEnum); pObject != NULL; pObject = GetEnumeratedObject(&sEnum))
        DestroyObjectByReference(pObject);
}

////////////////////////////////
//       Implementation       //
////////////////////////////////

sObjectEnumerator CreateObjectDirectoryEnumerator(sObjectDirectory *pDirectory)
{
    sObjectEnumerator sEnumerator;
    sEnumerator.bBucketIndex = 0;
    sEnumerator.pDirectory = pDirectory;
    sEnumerator.pDirectoryEntry = NULL;

    for (; sEnumerator.bBucketIndex < OBJECT_BUCKETS_PER_DIRECTORY; sEnumerator.bBucketIndex++)
    {
        sObjectNamespaceExtension *pEntry = pDirectory->arrEntryBucket[sEnumerator.bBucketIndex];
        
        while (pEntry->pObject == NULL)
        {
            pEntry = pEntry->pNextEntry;
            if (pEntry == NULL) break;
        }
        
        if (pEntry == NULL)
            continue;
        
        sEnumerator.pDirectoryEntry = pEntry;
        break;
    }
    
    return sEnumerator;
}

PVOID GetEnumeratedObject(sObjectEnumerator *pEnumerator)
{
    if (pEnumerator->pDirectoryEntry == NULL ||
        pEnumerator->pDirectory == NULL ||
        pEnumerator->pDirectory->dwNumEntries == 0)
        return NULL;

    PVOID pObject = pEnumerator->pDirectoryEntry->pObject;

    // Advance to the next directory entry
    pEnumerator->pDirectoryEntry = pEnumerator->pDirectoryEntry->pNextEntry;
    
    if (pEnumerator->pDirectoryEntry != NULL)
        return pObject;
    
    for (pEnumerator->bBucketIndex++; pEnumerator->bBucketIndex < OBJECT_BUCKETS_PER_DIRECTORY; pEnumerator->bBucketIndex++)
    {
        if (pEnumerator->pDirectory->arrEntryBucket[pEnumerator->bBucketIndex] == NULL) continue;
        pEnumerator->pDirectoryEntry = pEnumerator->pDirectory->arrEntryBucket[pEnumerator->bBucketIndex];
        break;
    }

    return pObject;
}

PVOID FindObjectInDirectory(sObjectDirectory *pDirectory, PWCHAR wszName)
{
    // Ugly code, but without it a heap allocation would be needed
    QWORD qwHash = HashPath(wszName, (pDirectory->bFlags & OBJECT_FLAG_IGNORE_CASE) != 0);
    
    for (sObjectNamespaceExtension *pEntry = pDirectory->arrEntryBucket[qwHash % OBJECT_BUCKETS_PER_DIRECTORY];
         pEntry != NULL; pEntry = pEntry->pNextEntry)
    {
        if (pEntry == NULL) break; // No such object exists
        if (pEntry->qwNameHash != qwHash) continue; // The hashes don't match
        if (strcmpW(pEntry->wsName.wszData, wszName)) continue; // Redundant check in case the hashes collide
        return pEntry->pObject;
    }
    
    return NULL;
}

sObjectCreationParameters MakeObjectCreationParameters(sString wsName, sObjectDirectory *pParent, BYTE bFlags)
{
    sObjectCreationParameters sParams;
    sParams.pParent = pParent;
    sParams.wsName  = wsName;
    sParams.bFlags  = bFlags;

    return sParams;
}

STATUS CreateObject(sObjectType *pType, sObjectCreationParameters *pParams, QWORD qwSize, PVOID *ppObject)
{
    if (qwSize < pType->qwObjectSize)
        return NEOS_INVALID_ARGUMENT | NEOS_ERROR_OBJECT;

    // Just a regular global object
    if (pParams == NULL)
    {
        // Calculate the final size of the object
        QWORD qwFinalSize = sizeof(sObjectHeader) + qwSize;
        
        sObjectHeader *pHeader = KHeapAlloc(qwFinalSize);
        if (pHeader == NULL)
            return NEOS_ALLOCATION_FAILURE | NEOS_ERROR_OBJECT;

        pHeader->bExtensions     = 0;
        pHeader->pObjectType     = pType;
        pHeader->qwNumHandles    = 0;
        pHeader->qwNumReferences = 1;

        ReferenceObject(pType);
        *ppObject = pHeader + 1;

        return NEOS_SUCCESS;
    }
    
    // if (FindObject(wszPath, NULL) != NULL) return NULL;

    PWCHAR wszPathDup                  = CopyStringAsCString(&pParams->wsName);
    sObjectDirectory *pDirectoryObject = pParams->pParent == NULL ? g_pRootDirectoryObject : pParams->pParent;
    PWCHAR wszCurrentName              = strtokW(wszPathDup, L"\\/");

    // FIXME: Add proper error handling in case a faulty path is entered
    while (wszCurrentName != NULL)
    {
        PVOID pObject = FindObjectInDirectory(pDirectoryObject, wszCurrentName);
        
        if (pObject == NULL)
            break;

        sObjectHeader *pHeader = (sObjectHeader *) pObject - 1;
        if (pHeader->pObjectType != g_pObjectDirectoryType)
            break;

        pDirectoryObject = pObject;
        wszCurrentName   = strtokW(NULL, L"\\/");
        if (wszCurrentName == NULL)
            break;
    }

    if (wszCurrentName == NULL)
        return NEOS_ERROR_OBJECT | NEOS_NOT_FOUND;

    // Calculate the final size of the object
    QWORD qwFinalSize = sizeof(sObjectNamespaceExtension) + sizeof(sObjectHeader) + qwSize;
    
    sObjectNamespaceExtension *pNameExt = KHeapAlloc(qwFinalSize);
    if (pNameExt == NULL)
        return NEOS_ALLOCATION_FAILURE | NEOS_ERROR_OBJECT;
    
    pNameExt->pParent    = pDirectoryObject;
    pNameExt->wsName     = MakeString(wszCurrentName);
    pNameExt->qwNameHash = HashPath(pNameExt->wsName.wszData, (pParams->bFlags & OBJECT_FLAG_IGNORE_CASE) != 0);
    pNameExt->pNextEntry = NULL;
    
    sObjectHeader *pHeader   = (sObjectHeader *) (pNameExt + 1);
    pHeader->bExtensions     = OBJECT_EXT_NAMESPACE;
    pHeader->pObjectType     = pType;
    pHeader->qwNumHandles    = 0;
    pHeader->qwNumReferences = 1;

    ReferenceObject(pType);
    *ppObject         = pHeader + 1;
    pNameExt->pObject = *ppObject;

    sObjectNamespaceExtension *pDirectoryEntry = pDirectoryObject->arrEntryBucket[pNameExt->qwNameHash % OBJECT_BUCKETS_PER_DIRECTORY];

    if (pDirectoryEntry == NULL) // This is the first entry
    {
        pDirectoryObject->arrEntryBucket[pNameExt->qwNameHash % OBJECT_BUCKETS_PER_DIRECTORY] = pNameExt;
        pDirectoryObject->dwNumEntries++;
    }
    else
    {
        while (pDirectoryEntry->pNextEntry != NULL)
            pDirectoryEntry = pDirectoryEntry->pNextEntry;

        pDirectoryEntry->pNextEntry = pNameExt;
        pDirectoryObject->dwNumEntries++;
    }

    return NEOS_SUCCESS;
}

PVOID FindObject(PWCHAR wszPath, sObjectDirectory *pParent)
{
    PWCHAR wszPathDup                  = strdupW(wszPath);
    sObjectDirectory *pDirectoryObject = pParent == NULL ? g_pRootDirectoryObject : pParent;
    PWCHAR wszCurrentName              = strtokW(wszPathDup, L"\\/");

    // FIXME: Add proper error handling in case a faulty path is entered
    while (wszCurrentName != NULL)
    {
        PVOID pObject = FindObjectInDirectory(pDirectoryObject, wszCurrentName);
        
        if (pObject == NULL)
            break;

        sObjectHeader *pHeader = (sObjectHeader *) pObject - 1;
        if (pHeader->pObjectType != g_pObjectDirectoryType)
        {
            KHeapFree(wszPathDup);
            return pObject;
        }

        pDirectoryObject = pObject;
        wszCurrentName   = strtokW(NULL, L"\\/");

        // This is the last part of the path
        if (wszCurrentName == NULL)
        {
            KHeapFree(wszPathDup);
            return pDirectoryObject;
        }
    }

    KHeapFree(wszPathDup);
    return NULL;
}

STATUS CreateObjectDirectory(sString wsPath, sObjectDirectory *pParent, sObjectDirectory **ppDirectoryObject)
{
    if (pParent == NULL)
        pParent = g_pRootDirectoryObject;

    sObjectDirectory *pDirectoryObject;

    sObjectCreationParameters sParams = MakeObjectCreationParameters(wsPath, pParent, 0);
    STATUS s = CreateObject(g_pObjectDirectoryType, &sParams, sizeof(sObjectDirectory), (PVOID *) &pDirectoryObject);

    if (!_SUCCESSFUL(s))
        return s;

    pDirectoryObject->pLock        = NULL;
    pDirectoryObject->bFlags       = 0;
    pDirectoryObject->dwNumEntries = 0;
    ZeroMemory(pDirectoryObject->arrEntryBucket, sizeof(pDirectoryObject->arrEntryBucket));

    if (ppDirectoryObject != NULL)
        *ppDirectoryObject = pDirectoryObject;

    return NEOS_SUCCESS;
}

sObjectType *CreateType(sString wsName, QWORD qwBodySize, void (*pDestroyCallback)(PVOID), void (*pParseCallback)(PVOID))
{
    _ASSERT(pDestroyCallback != NULL, L"CreateType expects pDestroyCallback to have a non-NULL value");
    sObjectCreationParameters sParameters = MakeObjectCreationParameters(wsName, g_pTypeDirectory, 0);
    sObjectType *pResultType;

    STATUS status = CreateObject(g_pObjectType, &sParameters, sizeof(sObjectType), (PVOID) &pResultType);
    pResultType->pDestroyCallback = pDestroyCallback;
    pResultType->pParseCallback   = pParseCallback;
    pResultType->qwObjectSize     = qwBodySize;
    pResultType->wsName           = wsName;

    if (!_SUCCESSFUL(status))
    {
        ArchLog(LOG_WARNING, L"CreateType should return a STATUS");
        return NULL;
    }

    return pResultType;
}

void DestroyType(sObjectType *pType)
{
    DestroyObjectByReference(pType);
}

void DestroyObjectByReference(PVOID pObject)
{
    if (pObject == NULL) 
        return;
    
    sObjectHeader *pHeader = (sObjectHeader *) pObject - 1;

    if (pHeader->bExtensions & OBJECT_EXT_NAMESPACE)
    {
        // Remove the object from its parent directory

        sObjectNamespaceExtension *pExtension = (sObjectNamespaceExtension *) pHeader - 1;
        pExtension->pParent->dwNumEntries--;

        for (sObjectNamespaceExtension *pEntry = pExtension->pParent->arrEntryBucket[pExtension->qwNameHash % OBJECT_BUCKETS_PER_DIRECTORY];
             pEntry != NULL; pEntry = pEntry->pNextEntry)
        {
            if (pEntry->pNextEntry != pExtension) continue;
            // Cut out the entry to be destroyed
            pEntry->pNextEntry = pEntry->pNextEntry->pNextEntry;
            break;
        }

    }
    
    if (pHeader->pObjectType->pDestroyCallback != NULL)
        pHeader->pObjectType->pDestroyCallback(pObject);

    if (pHeader->bExtensions == 0)
        KHeapFree(pHeader);
    else if (pHeader->bExtensions == OBJECT_EXT_NAMESPACE)
        KHeapFree((sObjectNamespaceExtension *) pHeader - 1);
}

void DestroyObject(PWCHAR wszPath)
{
    DestroyObjectByReference(FindObject(wszPath, NULL));
}

void ReferenceObject(PVOID pObject)
{
    ((sObjectHeader *) pObject - 1)->qwNumReferences++;
}

void DereferenceObject(PVOID pObject)
{
    // If the decremented value is 0, delete the object
    if (--((sObjectHeader *) pObject - 1)->qwNumReferences == 0)
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

HANDLE AllocateHandle(sHandleTable *pTable, PVOID pObject)
{
    sHandleEntry sEntry;
    sEntry.pObject = pObject;
    sEntry.hHandle = pTable->qwCurrentHandleNumber++;
    ReferenceObject(pObject);
    AddListElement(&pTable->lstHandles, &sEntry);
    return sEntry.hHandle;
}

PVOID GetObjectFromHandle(sHandleTable *pTable, HANDLE hHandle)
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

void PrintObjectTree(sObjectDirectory *pDirectory, DWORD dwCurrentDepth)
{
    if (pDirectory == NULL && dwCurrentDepth == 0)
    {
        pDirectory = g_pRootDirectoryObject;
        ArchPrintFormat(L"<root>\n");
    }
    else if (dwCurrentDepth == 0)
    {
        sObjectNamespaceExtension *pExtension = (sObjectNamespaceExtension *) ((PBYTE) pDirectory - sizeof(sObjectHeader) - sizeof(sObjectNamespaceExtension));
        ArchPrintFormat(L"%S\n", pExtension->wsName.wszData);
    }
    
    sObjectEnumerator sEnumerator = CreateObjectDirectoryEnumerator(pDirectory);
    for (PVOID pObject = GetEnumeratedObject(&sEnumerator); pObject != NULL; pObject = GetEnumeratedObject(&sEnumerator))
    {
        ArchPrintChar('\xB3'); // │
        ArchSetCursor(dwCurrentDepth * 2, ArchGetCursorY());

        // if (i == pDirectory->lstChildren.qwLength - 1)
        //     ArchPrintFormat(L"\xC0\xC4"); // └─
        // else
            ArchPrintFormat(L"\xC3\xC4"); // ├─
        
        sObjectHeader *pHeader = (sObjectHeader *) pObject - 1;
        sObjectNamespaceExtension *pExtension = (sObjectNamespaceExtension *) pHeader - 1;
        ArchPrintFormat(L"%S\n", pExtension->wsName.wszData);

        if (pHeader->pObjectType == g_pObjectDirectoryType)
            PrintObjectTree(pDirectory, dwCurrentDepth + 1);
    }
}

STATUS InitObjectManager()
{
    // Initalise the Type type
    sObjectHeader *pTypeObjectHeader = KHeapAlloc(sizeof(sObjectHeader) + sizeof(sObjectType));
    pTypeObjectHeader->bExtensions = 0;
    pTypeObjectHeader->pExtra = NULL;
    pTypeObjectHeader->pObjectType = (sObjectType *) (pTypeObjectHeader + 1);
    pTypeObjectHeader->qwNumHandles = 0;
    pTypeObjectHeader->qwNumReferences = 1;
    g_pObjectType = (sObjectType *) (pTypeObjectHeader + 1);

    g_pObjectType->wsName               = MakeString(L"Type");
    g_pObjectType->qwObjectSize         = sizeof(sObjectType);
    g_pObjectType->pDestroyCallback     = DestroyTypeCallback;
    g_pObjectType->pParseCallback       = NULL;

    STATUS s = CreateObject(g_pObjectType, NULL, sizeof(sObjectType), (PVOID *) &g_pObjectDirectoryType);
    if (!_SUCCESSFUL(s))
        return s;
    
    g_pObjectDirectoryType->pDestroyCallback     = DestroyDirectoryCallback;
    g_pObjectDirectoryType->pParseCallback       = NULL;
    g_pObjectDirectoryType->wsName               = MakeString(L"Directory");
    g_pObjectDirectoryType->qwObjectSize         = sizeof(sObjectDirectory);

    s = CreateObject(g_pObjectDirectoryType, NULL, sizeof(sObjectDirectory), (PVOID) &g_pRootDirectoryObject);
    if (!_SUCCESSFUL(s))
        return s;

    ZeroMemory(g_pRootDirectoryObject->arrEntryBucket, sizeof(g_pRootDirectoryObject->arrEntryBucket));
    g_pRootDirectoryObject->bFlags       = 0;
    g_pRootDirectoryObject->dwNumEntries = 0;
    g_pRootDirectoryObject->pLock        = NULL;
    
    // Initialise directories
    s = CreateObjectDirectory(MakeString(L"ObjectTypes"), NULL, &g_pTypeDirectory);
    if (!_SUCCESSFUL(s))
        return s;

    return NEOS_SUCCESS;
}
