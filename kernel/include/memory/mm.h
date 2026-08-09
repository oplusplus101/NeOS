
#ifndef __MEMORY__MM_H
#define __MEMORY__MM_H

#include <common/types.h>

#define PAGE_CACHE_RADIX 9
#define PAGE_CAHCE_NUM_ENTREIS (1 << PAGE_CACHE_RADIX)

typedef struct
{
    PVOID arrChildren[PAGE_CAHCE_NUM_ENTREIS];
} __attribute__((packed)) sPageCacheNode;

typedef struct
{
    PHYSICAL_ADDRESS PageFrame;
    DWORD dwReferenceCount;
} sPageCacheLeaf;

typedef struct
{
    enum : BYTE
    {
        MM_FILE,
        MM_ANONYMOUS,
        MM_SHARED,
        MM_DEVICE,
        MM_IMAGE
    } bType;

    PVOID pBackend; // Used for files
    PVOID pLock;

    sPageCacheNode *pPageCache;   // cached physical pages

    SIZE szLength;
    WORD wPermissions;
} sMemoryObject;

typedef struct _tagMemoryRegion
{
    VIRTUAL_ADDRESS Base;
    VIRTUAL_ADDRESS ObjectOffset;
    sMemoryObject  *pMemoryObject;

    // Tree stuff
    struct _tagMemoryRegion *pParent;
    struct _tagMemoryRegion *pLeft;
    struct _tagMemoryRegion *pRight;
    BOOL                     bIsRed;
} sMemoryRegion;

typedef struct
{
    PVOID pPageTable;
    sMemoryRegion *pRegionTreeRoot;
    PVOID pLock; // for future use.
} sAddressSpace;

STATUS          MMAddMemoryRegion(sAddressSpace *pAddressSpace, VIRTUAL_ADDRESS Base, VIRTUAL_ADDRESS ObjectOffset, sMemoryObject *pMemoryObject);
STATUS          MMCreateAddressSpace(sAddressSpace **ppAddressSpace);
STATUS          MMCreateMemoryObject(BYTE bType, PVOID pBackend, WORD wPermissions, SIZE szLength, sMemoryObject **ppObject);
sPageCacheLeaf *MMLookupMemoryObject(sMemoryObject *pMemoryObject, DWORD dwPageIndex);
BOOL            MMAddPageToMemoryObject(sMemoryObject *pMemoryObject, DWORD dwPageIndex, PHYSICAL_ADDRESS PageFrame);
sMemoryRegion  *MMFindMemoryRegion(sAddressSpace *pAddressSpace, VIRTUAL_ADDRESS Address);
sPageCacheLeaf *MMLookupPageInMemoryObject(sMemoryObject *pMemoryObject, DWORD dwPageIndex);
sAddressSpace  *MMGetCurrentAddressSpace();
void            MMSetCurrentAddressSpace(sAddressSpace *pAddressSpace);



STATUS          InitialiseMemoryManager();

#endif // __MEMORY__MM_H
