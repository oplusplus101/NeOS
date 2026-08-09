
#include <memory/mm.h>
#include <memory/heap.h>
#include <runtime/objects.h>
#include <common/memory.h>
#include <neos.h>
#include <archimports.h>
#include <memory/pmm.h>
#include <runtime/exe.h>

sObjectType *g_pMemoryObjectType;
sObjectType *g_pAddressSpaceType;

sAddressSpace *g_pCurrentAddressSpace;

void SYSV PageFault(VIRTUAL_ADDRESS FaultAddress, WORD wFlags, BOOL bInstructionFetch)
{
    // Invalid access
    if (FaultAddress < PAGE_SIZE)
    {
        ArchLog(LOG_GOODBYE, L"Page fault! Tried to %S memory address 0x%p", wFlags & PAGING_WRITEABLE ? L"write" : L"read", FaultAddress);
        for (;;);
    }

    sMemoryRegion *pRegion = MMFindMemoryRegion(g_pCurrentAddressSpace, FaultAddress);

    if (pRegion == NULL)
    {
        ArchLog(LOG_GOODBYE, L"Page fault! Tried to %S memory from non-existent region 0x%p", wFlags & PAGING_WRITEABLE ? L"write" : L"read", FaultAddress);
        for (;;);
    }

    DWORD dwPageIndex = ((FaultAddress & ~(PAGE_SIZE - 1)) - pRegion->Base + pRegion->ObjectOffset) / PAGE_SIZE;
    
    sPageCacheLeaf *pLeaf = MMLookupPageInMemoryObject(pRegion->pMemoryObject, dwPageIndex);

    switch (pRegion->pMemoryObject->bType)
    {
    case MM_ANONYMOUS:
    {
        PHYSICAL_ADDRESS PageFrame = PMMAllocatePage();
        ArchMapPage(g_pCurrentAddressSpace->pPageTable, FaultAddress & ~(PAGE_SIZE - 1), PageFrame, pRegion->pMemoryObject->wPermissions, PMMAllocatePage);
        MMAddPageToMemoryObject(pRegion->pMemoryObject, dwPageIndex, PageFrame);
        break;
    }
    case MM_IMAGE:
    {
        sImageSection *pSection = pRegion->pMemoryObject->pBackend;
        VIRTUAL_ADDRESS OffsetWithinSection = (FaultAddress - pSection->pImage->ImageBase - pSection->RelativeVirtualStart) & ~(PAGE_SIZE - 1);
        PHYSICAL_ADDRESS PageFrame = PMMAllocatePage();
        if (PageFrame == 0)
        {
            ArchLog(LOG_GOODBYE, L"Failed to allocate a page for image");
            for (;;);
        }

        // Temporary mapping, so that data can be written
        ArchMapPage(NULL, FaultAddress & ~(PAGE_SIZE - 1), PageFrame, PAGING_WRITEABLE | PAGING_PRESENT, PMMAllocatePage);
        // TODO: Check pImage and pBlockDevice in case they're NULL
        pSection->pImage->pBlockDevice->Read(pSection->pImage->pBlockDevice, pSection->qwRawOffset + OffsetWithinSection, PAGE_SIZE, (PVOID) (FaultAddress & ~(PAGE_SIZE - 1)));

        STATUS s = RelocateImagePage(pSection->pImage, FaultAddress & ~(PAGE_SIZE - 1));
        if (!_SUCCESSFUL(s))
        {
            // TODO: Kill any driver/process that failed
            ArchLog(LOG_ERROR, L"Relocation failed");
            for (;;);
        }
        s = LoadImports(pSection->pImage);
        if (!_SUCCESSFUL(s))
        {
            // TODO: Kill any driver/process that failed
            ArchLog(LOG_ERROR, L"Import load failed");
            for (;;);
        }

        ArchMapPage(g_pCurrentAddressSpace->pPageTable, FaultAddress & ~(PAGE_SIZE - 1), PageFrame, pRegion->pMemoryObject->wPermissions, PMMAllocatePage);
        MMAddPageToMemoryObject(pRegion->pMemoryObject, dwPageIndex, PageFrame);
        break;
    }
    
    default:
        ArchLog(LOG_GOODBYE, L"Memory region type %u not implemented", pRegion->pMemoryObject->bType);
        for (;;);
        break;
    }
}

static void DestoryMemoryObjectCallback(PVOID pObject)
{
    // TODO: Free the page table
    
    // sMemoryObject *pMemoryObject = pObject;
    // pMemoryObject->
}

static void DestroyAddressSpaceCallback(PVOID pObject)
{
    sAddressSpace *pAddressSpace = pObject;
    KHeapFree(pAddressSpace->pRegionTreeRoot);
}

static sMemoryRegion* RotateSubtree(sAddressSpace *pTree, sMemoryRegion *pSub, BOOL bRight)
{
    sMemoryRegion *pSubParent = pSub->pParent;
    sMemoryRegion *pNewChild, *pNewRoot;// = sub->child[1 - dir]; // 1 - dir is the opposite direction

    if (bRight)
    {
        pNewRoot    = pSub->pLeft;
        pNewChild   = pNewRoot->pRight;
        pSub->pLeft = pNewChild;
    }
    else
    {
        pNewRoot     = pSub->pRight;
        pNewChild    = pNewRoot->pLeft;
        pSub->pRight = pNewChild;
    }

    if (pNewChild)
        pNewChild->pParent = pSub;

    if (bRight)
        pNewRoot->pRight   = pSub;
    else
        pNewRoot->pLeft    = pSub;

    pNewRoot->pParent = pSubParent;
    pSub->pParent     = pNewRoot;

    if (pSubParent)
    {
        if (pSubParent == pSubParent->pRight)
            pSubParent->pRight = pNewRoot;
        else
            pSubParent->pLeft  = pNewRoot;
    }
    else
        pTree->pRegionTreeRoot = pNewRoot;

    return pNewRoot;
}

// parent is optional
static void Insert(sAddressSpace *pSpace, sMemoryRegion *pNode, sMemoryRegion *pParent, BOOL bRight)
{
    pNode->bIsRed  = true;
    pNode->pParent = pParent;

    if (!pParent)
    {
        pSpace->pRegionTreeRoot = pNode;
        return;
    }

    if (bRight) pParent->pRight = pNode;
    else        pParent->pLeft  = pNode;

    // rebalance the tree 
    do
    {
        // Case #1
        if (!pParent->bIsRed)
            return; 

        sMemoryRegion* pGrandparent = pParent->pParent;

        if (!pGrandparent)
        {
            // Case #4
            pParent->bIsRed = false;
            return;
        }

        bRight = pParent == pParent->pParent->pRight;

        sMemoryRegion* pUncle = bRight ? pGrandparent->pLeft : pGrandparent->pRight;

        if (!pUncle || !pUncle->bIsRed)
        {
            if (pNode == (bRight ? pGrandparent->pLeft : pGrandparent->pRight))
            {
                // Case #5
                RotateSubtree(pSpace, pParent, bRight);
                pNode = pParent;
                pParent = bRight ? pGrandparent->pRight : pGrandparent->pLeft;
            }

            // Case #6
            RotateSubtree(pSpace, pGrandparent, !bRight);
            pParent->bIsRed = false;
            pGrandparent->bIsRed = true;
            return;
        }
    
        // Case #2
        pParent->bIsRed      = false;
        pUncle->bIsRed        = false;
        pGrandparent->bIsRed = true;
        pNode = pGrandparent;

    } while ((pParent = pNode->pParent));
}

// Returns false in case a duplicate is passed
static BOOL InsertIntoRegionTree(sAddressSpace *pAddressSpace, sMemoryRegion *pNode)
{
    sMemoryRegion *pCurrentRoot = pAddressSpace->pRegionTreeRoot;
    if (pCurrentRoot == NULL)
    {
        pNode->bIsRed    = true;
        pNode->pLeft     = NULL;
        pNode->pRight    = NULL;
        pNode->pParent   = pCurrentRoot;
        pAddressSpace->pRegionTreeRoot = pNode;
        return true;
    }
    // if (pCurrentRoot == NULL) return false;

    while (pCurrentRoot != NULL)
    {
        // Insert it to the left
        if (pCurrentRoot->Base > pNode->Base)
        {
            if (pCurrentRoot->pLeft == NULL) break;
            pCurrentRoot = pCurrentRoot->pLeft;
        }
        else if (pCurrentRoot->Base < pNode->Base) // Insert it to the right
        {
            if (pCurrentRoot->pRight == NULL) break;
            pCurrentRoot = pCurrentRoot->pRight;
        }
        else
            return false; // Duplicate
    }

    pNode->bIsRed    = true;
    pNode->pLeft     = NULL;
    pNode->pRight    = NULL;
    pNode->pParent   = pCurrentRoot;

    // Temporarily use a regular binary tree
    if (pNode->Base > pCurrentRoot->Base)
        pCurrentRoot->pRight = pNode;
    else
        pCurrentRoot->pLeft = pNode;
    
    // Insert(pAddressSpace, pNode, pCurrentRoot, pNode->Base > pCurrentRoot->Base);
        
    return true;
}

STATUS InitialiseMemoryManager()
{
    g_pMemoryObjectType = CreateType(MakeString(L"MemoryObject"), sizeof(sMemoryObject), DestoryMemoryObjectCallback, NULL);
    g_pAddressSpaceType = CreateType(MakeString(L"AddressSpace"), sizeof(sAddressSpace), DestroyAddressSpaceCallback, NULL);

    ArchRegisterPageFaultHandler(PageFault);

    g_pCurrentAddressSpace                           = KHeapAlloc(sizeof(sAddressSpace));
    if (g_pCurrentAddressSpace == NULL)
        return NEOS_ALLOCATION_FAILURE;
    g_pCurrentAddressSpace->pLock                    = NULL;
    g_pCurrentAddressSpace->pPageTable               = ArchGetKernelPageTable();
    
    // g_pCurrentAddressSpace->pRegionTreeRoot          = KHeapAlloc(sizeof(sMemoryRegion));
    // if (g_pCurrentAddressSpace->pRegionTreeRoot == NULL)
        // return NEOS_ALLOCATION_FAILURE;

    // g_pCurrentAddressSpace->pRegionTreeRoot->pParent = NULL;
    // g_pCurrentAddressSpace->pRegionTreeRoot->bIsRed  = false;
}

STATUS MMAddMemoryRegion(sAddressSpace *pAddressSpace, VIRTUAL_ADDRESS Base, VIRTUAL_ADDRESS ObjectOffset, sMemoryObject *pMemoryObject)
{
    sMemoryRegion *pChild = KHeapAlloc(sizeof(sMemoryRegion));

    if (pChild == NULL) return NEOS_ALLOCATION_FAILURE;

    pChild->Base          = Base;
    pChild->ObjectOffset  = ObjectOffset;
    pChild->pMemoryObject = pMemoryObject;

    // Tree stuff
    pChild->pParent = NULL;
    pChild->pLeft   = NULL;
    pChild->pRight  = NULL;
    pChild->bIsRed  = true;

    BOOL bWorked = InsertIntoRegionTree(pAddressSpace, pChild);
    if (!bWorked) return NEOS_FAILURE;
    ReferenceObject(pMemoryObject);

    return NEOS_SUCCESS;
}

STATUS MMResizeMemoryRegion()
{

}


STATUS MMCreateAddressSpace(sAddressSpace **ppAddressSpace)
{
    STATUS s = CreateObject(g_pAddressSpaceType, NULL, sizeof(sMemoryObject), (PVOID *) ppAddressSpace);
    if (!_SUCCESSFUL(s))
        return s;

    // (*ppAddressSpace)->pRegionTreeRoot          = KHeapAlloc(sizeof(sMemoryRegion));
    // (*ppAddressSpace)->pRegionTreeRoot->pParent = NULL;
    // (*ppAddressSpace)->pRegionTreeRoot->bIsRed  = false;

    (*ppAddressSpace)->pPageTable               = ArchCloneKernelPageTable(PMMAllocatePage);
}


// Memory Object

STATUS MMCreateMemoryObject(BYTE bType, PVOID pBackend, WORD wPermissions, SIZE szLength, sMemoryObject **ppObject)
{
    STATUS s = CreateObject(g_pMemoryObjectType, NULL, sizeof(sMemoryObject), (PVOID *) ppObject);
    if (!_SUCCESSFUL(s))
        return s;
    
    (*ppObject)->bType        = bType;
    (*ppObject)->pBackend     = pBackend;
    (*ppObject)->pLock        = NULL;
    (*ppObject)->szLength     = szLength;
    (*ppObject)->wPermissions = wPermissions;
    (*ppObject)->pPageCache   = KHeapAlloc(sizeof(sPageCacheNode));
    if ((*ppObject)->pPageCache == NULL)
        return NEOS_ALLOCATION_FAILURE;
    ZeroMemory((*ppObject)->pPageCache, sizeof(sPageCacheNode));

    return NEOS_SUCCESS;
}

sMemoryRegion *MMFindMemoryRegion(sAddressSpace *pAddressSpace, VIRTUAL_ADDRESS Address)
{
    sMemoryRegion *pCurrentRegion = pAddressSpace->pRegionTreeRoot;

    for (;;)
    {
        if (pCurrentRegion == NULL) return NULL;

        if (Address >= pCurrentRegion->Base && Address < pCurrentRegion->Base + pCurrentRegion->pMemoryObject->szLength)
            return pCurrentRegion;
       
        if (pCurrentRegion->pLeft == NULL && pCurrentRegion->pRight == NULL)
            break;

        if (Address > pCurrentRegion->Base)
            pCurrentRegion = pCurrentRegion->pRight;
        else
            pCurrentRegion = pCurrentRegion->pLeft;
    }
    

    return NULL;
}

sPageCacheLeaf *MMLookupPageInMemoryObject(sMemoryObject *pMemoryObject, DWORD dwPageIndex)
{
    WORD wL0 = (dwPageIndex >> 18) & ~(PAGE_CAHCE_NUM_ENTREIS - 1);
    WORD wL1 = (dwPageIndex >> 9) & ~(PAGE_CAHCE_NUM_ENTREIS - 1);
    WORD wL2 = dwPageIndex & ~(PAGE_CAHCE_NUM_ENTREIS - 1);

    sPageCacheNode *pL1Node = pMemoryObject->pPageCache->arrChildren[wL0];
    if (pL1Node == NULL)
        return NULL;
    
    sPageCacheNode *pL2Node = pL1Node->arrChildren[wL1];
    if (pL2Node == NULL)
        return NULL;
    
    return pL2Node->arrChildren[wL1];
}

BOOL MMAddPageToMemoryObject(sMemoryObject *pMemoryObject, DWORD dwPageIndex, PHYSICAL_ADDRESS PageFrame)
{
    WORD wL0 = (dwPageIndex >> 18) & ~(PAGE_CAHCE_NUM_ENTREIS - 1);
    WORD wL1 = (dwPageIndex >> 9) & ~(PAGE_CAHCE_NUM_ENTREIS - 1);
    WORD wL2 = dwPageIndex & ~(PAGE_CAHCE_NUM_ENTREIS - 1);

    sPageCacheNode *pL1Node = pMemoryObject->pPageCache->arrChildren[wL0];
    if (pL1Node == NULL)
    {
        pL1Node = KHeapAlloc(sizeof(sPageCacheNode));
        if (pL1Node == NULL)
            return false;
        ZeroMemory(pL1Node, sizeof(sPageCacheNode));
        pMemoryObject->pPageCache->arrChildren[wL0] = pL1Node;
    }

    sPageCacheNode *pL2Node = pL1Node->arrChildren[wL1];
    if (pL2Node == NULL)
    {
        pL2Node = KHeapAlloc(sizeof(sPageCacheLeaf) * PAGE_CAHCE_NUM_ENTREIS);
        if (pL2Node == NULL)
            return false;
        ZeroMemory(pL2Node, sizeof(sPageCacheLeaf) * PAGE_CAHCE_NUM_ENTREIS);
        pL1Node->arrChildren[wL1] = pL2Node;
    }
    
    (((sPageCacheLeaf *) pL2Node->arrChildren)[wL2]).PageFrame        = PageFrame;
    (((sPageCacheLeaf *) pL2Node->arrChildren)[wL2]).dwReferenceCount = 0;
    return true;
}

sAddressSpace *MMGetCurrentAddressSpace()
{
    return g_pCurrentAddressSpace;
}

void MMSetCurrentAddressSpace(sAddressSpace *pAddressSpace)
{
    g_pCurrentAddressSpace = pAddressSpace;
}

