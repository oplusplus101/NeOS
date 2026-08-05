
#ifndef __COMMON__INI_H
#define __COMMON__INI_H

#include <common/types.h>
#include <common/list.h>

typedef struct
{
    CHAR szLabel[256]; // The value enclosed in brackets (eg. [NEOS])
    CHAR szName[256];  // The identifier before the equals sign
    CHAR szValue[256];  // The value
} sINIEntry;

sList ParseINIFile(PCHAR szContents);

#endif // __COMMON__INI_H
