
#ifndef __RUNTIME__OBJECTS_HH
#define __RUNTIME__OBJECTS_HH

#include <common/types.h>
#include <common/list.h>
#include <common/countedstring.h>

#define OBJECT_NAME_LENGTH 256
#define OBJECT_BUCKETS_PER_DIRECTORY 31

#define OBJECT_EXT_NAMESPACE 1
#define OBJECT_EXT_SECURITY  2

#define OBJECT_FLAG_IGNORE_CASE 1

struct _tagObjectDirectory;

typedef struct _tagObjectNamespaceExtension
{
    sString wsName;
    struct _tagObjectDirectory *pParent;
    struct _tagObjectNamespaceExtension *pNextEntry;
    QWORD qwNameHash;
    PVOID pObject;
} sObjectNamespaceExtension;

typedef struct _tagObjectDirectory
{
    sObjectNamespaceExtension *arrEntryBucket[OBJECT_BUCKETS_PER_DIRECTORY];
    PVOID pLock;
    BYTE  bFlags;
    DWORD dwNumEntries;
} sObjectDirectory;


typedef struct
{
    BYTE bBucketIndex;
    sObjectNamespaceExtension *pDirectoryEntry;
    sObjectDirectory *pDirectory;
} sObjectEnumerator;

typedef struct
{
    sString wsName;
    QWORD qwObjectSize;
    void (*pDestroyCallback)(PVOID);
    void (*pParseCallback)(PVOID);
} sObjectType;

typedef struct
{
    BYTE bExtensions;
    QWORD qwNumHandles, qwNumReferences;
    sObjectType *pObjectType;
    PVOID *pExtra; // Optional data (might be useful later)
} sObjectHeader;

typedef struct
{
    sString wsName;
    sObjectDirectory *pParent;
    BYTE bFlags;
} sObjectCreationParameters;

typedef struct
{
    sList lstHandles;
    QWORD qwCurrentHandleNumber;
} sHandleTable;

typedef struct
{
    HANDLE hHandle;
    PVOID pObject;
} sHandleEntry;


/// @brief Creates an enumerator to be used with GetEnumeratedObject
/// @param pDirectory The directory to enumerate
/// @return An enumerator object
sObjectEnumerator CreateObjectDirectoryEnumerator(sObjectDirectory *pDirectory);

/// @brief Gets the next object from a directory
/// @param pEnumerator A directory enumerator from CreateObjectDirectoryEnumerator
/// @return The object upon success, NULL otherwise.
PVOID GetEnumeratedObject(sObjectEnumerator *pEnumerator);

PVOID FindObjectInDirectory(sObjectDirectory *pDirectory, PWCHAR wszName);
PVOID FindObject(PWCHAR wszPath, sObjectDirectory *pParent);

/// @brief Creates a directory at the specified path relative to pParent
/// @param wszPath The path of the directory
/// @param pParent The parent of the directory (can be set to NULL for an absolute path)
/// @param ppDirectoryObject The returned directory object (can be set to NULL)
/// @return A STATUS
STATUS CreateObjectDirectory(sString wszPath, sObjectDirectory *pParent, sObjectDirectory **ppDirectoryObject);



/// @brief Creates a type
/// @param wsName The type's name
/// @param qwBodySize The minimum size of the object's body
/// @param pDestroyCallback A callback for when the object gets destroyed
/// @param pParseCallback OPTIONAL: A callback if the object need special handling in the namespace for resolvihng paths
/// @return A pointer to the type
sObjectType *CreateType(sString wsName, QWORD qwBodySize, void (*pDestroyCallback)(PVOID), void (*pParseCallback)(PVOID));
void         DestroyType(sObjectType *pType);

/// @brief Creates a struct to be passed into CreateObject with appropriate parameters
/// @param wsName The objects path relative to pParent
/// @param pParent The object's parent. Can be set to NULL for a global path
/// @param bFlags Extra flags. Can be set to 0.
/// @return The resulting struct
sObjectCreationParameters MakeObjectCreationParameters(sString wsName, sObjectDirectory *pParent, BYTE bFlags);

/// @brief Creates an object
/// @param pType The object's type
/// @param pParams Optional parameters like path in the namespace
/// @param qwSize Allocation size for the object. Must be greater than or equal to the type's specified size
/// @param ppObject The returned object
/// @return A STATUS
STATUS CreateObject(sObjectType *pType, sObjectCreationParameters *pParams, QWORD qwSize, PVOID *ppObject);

void         DestroyObjectByReference(PVOID pObject);
void         DestroyObject(PWCHAR wszPath);

/// @brief Increments the object's reference count
void         ReferenceObject(PVOID pObject);

/// @brief Decrements the object's reference count. If the reference count hits 0, the object gets destroyed.
void         DereferenceObject(PVOID pObject);


sHandleTable CreateHandleTable();
void         DestroyHandleTable(sHandleTable *pTable);

/// @brief Inserts the handle inside pTable and increments the refrence count of pObject
/// @return The handle number upon success, zero otherwise
INT          AllocateHandle(sHandleTable *pTable, PVOID pObject);

/// @brief Retieves an object pointer from a handle. This doesn't affect the object's reference count
/// @return The object's pointer upon success, NULL otherwise
PVOID        GetObjectFromHandle(sHandleTable *pTable, HANDLE hHandle);
void         FreeHandle(sHandleTable *pTable, HANDLE hHandle);

/// @brief Prints all the entries in the object directory similar to the UNIX tree command
/// @param dwCurrentDepth Internal variable. Set this to 0
void         PrintObjectTree(sObjectDirectory *pObject, DWORD dwCurrentDepth);

STATUS       InitObjectManager();

#endif // __RUNTIME__OBJECTS_HH
