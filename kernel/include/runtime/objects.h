
#ifndef __RUNTIME__OBJECTS_HH
#define __RUNTIME__OBJECTS_HH

#include <common/types.h>
#include <memory/list.h>

typedef struct
{
    WCHAR wszName[257];
    QWORD qwBodySize;
    void (*pDestroyCallback)(PVOID);
} sObjectType;

typedef struct
{
    sList lstChildren;
} sObjectDirectory;

typedef struct _tagObject
{
    WCHAR wszName[257];
    DWORD dwReferenceCount;
    sObjectType *pType;
    struct _tagObject *pParent;
    PVOID pBody;
} sObject;

typedef struct
{
    sList lstHandles;
    QWORD qwCurrentHandleNumber;
} sHandleTable;

typedef struct
{
    INT iHandle;
    sObject *pObject;
} sHandleEntry;

void         DestroyDirectoryCallback(sObject *pObject);
BOOL         AddObjectToDirectory(sObject *pDirectory, sObject *pObject);
sObject     *FindObjectInDirectory(sObjectDirectory *pDirectory, PWCHAR wszName);
sObject     *LookupObject(PWCHAR wszPath);
sObject     *CreateObjectDirectory(PWCHAR wszPath);
sObjectType *CreateType(PWCHAR wszName, QWORD qwBodySize, void (*pDestroyCallback)(sObject *));
void         DestroyType(sObjectType *pType);
sObject     *CreateObject(PWCHAR wszPath, sObjectType *pType, PVOID pBody);
void         DestroyObjectByReference(sObject *pObject);
void         DestroyObject(PWCHAR wszPath);
void         ReferenceObject(sObject *pObject);
void         DereferenceObject(sObject *pObject);
sHandleTable CreateHandleTable();
void         DestroyHandleTable(sHandleTable *pTable);
INT          AllocateHandle(sHandleTable *pTable, sObject *pObject);
sObject     *GetObjectFromHandle(sHandleTable *pTable, INT iHandle);
void         FreeHandle(sHandleTable *pTable, INT iHandle);
void         PrintObjectTree(sObject *pObject, DWORD dwCurrentDepth);

void InitObjectManager();

#endif // __RUNTIME__OBJECTS_HH
