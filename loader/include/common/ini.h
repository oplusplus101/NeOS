
#ifndef __INI_H
#define __INI_H

#include <memory/list.h>
#include <common/types.h>

typedef struct
{
    CHAR szLabel[256]; // The value enclosed in brackets (eg. [NEOS])
    CHAR szName[256];  // The identifier before the equals sign
    CHAR szValue[256];  // The value
} sINIEntry;

sList ParseINIFile(PCHAR szContents);

#endif // __INI_H
