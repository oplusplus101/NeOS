
#include <NeoLinkedList.h>

void InitLinkedList(sLinkedListEntry *pEntry)
{
    pEntry->pForwards  = pEntry;
    pEntry->pBackwards = pEntry;
}

BOOL IsListEmpty(sLinkedListEntry *pFirstEntry)
{
    return pFirstEntry->pBackwards == pFirstEntry->pForwards;
}

void PrependIntoLinkedList(sLinkedListEntry *pFirstEntry, sLinkedListEntry *pEntry)
{
    pEntry->pBackwards      = pFirstEntry->pBackwards;
    pEntry->pForwards       = pFirstEntry;
    pFirstEntry->pBackwards = pEntry;
}

void AppendIntoLinkedList(sLinkedListEntry *pFirstEntry, sLinkedListEntry *pEntry)
{
    pEntry->pForwards                  = pFirstEntry;
    pEntry->pBackwards                 = pFirstEntry->pBackwards;
    pFirstEntry->pBackwards->pForwards = pEntry;
    pFirstEntry->pBackwards            = pEntry;
}

void RemoveFromLinkedList(sLinkedListEntry *pEntry)
{
    pEntry->pBackwards = pEntry->pForwards;
}

