
#ifndef __NEOLINKEDLIST_H
#define __NEOLINKEDLIST_H

#include <NeoTypes.h>


typedef struct _tagLinkedListEntry
{
    struct _tagLinkedListEntry *pBackwards;
    struct _tagLinkedListEntry *pForwards;
} sLinkedListEntry;

#define _GET_LINKED_LIST_DATA(pList, structName, listFieldName) \
    ((structName *)((PCHAR)(pList) - __builtin_offsetof(structName, listFieldName)))

/// @brief Set pForwards and pBackwards to the address of pListEntry
/// @param pFirstEntry The first element of the list to be initialised
void InitLinkedList(sLinkedListEntry *pFirstEntry);

/// @brief Checks whether a list is empty
/// @param pFirstEntry The first element of the list
/// @return True if empty, false otherwise
BOOL IsListEmpty(sLinkedListEntry *pFirstEntry);

/// @brief Inserts pEntry to the beginning of the list
/// @param pFirstEntry The first element of the list
/// @param pEntry The entry to be inserted
void PrependIntoLinkedList(sLinkedListEntry *pFirstEntry, sLinkedListEntry *pEntry);

/// @brief Inserts pEntry to the end of the list
/// @param pFirstEntry The first element of the list

/// @param pEntry The entry to be inserted
void AppendIntoLinkedList(sLinkedListEntry *pFirstEntry, sLinkedListEntry *pEntry);

/// @brief Slices the specified entry out of a list
/// @param pEntry The entry to removed
void RemoveFromLinkedList(sLinkedListEntry *pEntry);

#endif // __NEOLINKEDLIST_H
