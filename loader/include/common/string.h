
#ifndef __STRING_H
#define __STRING_H

#include <common/types.h>

QWORD strlen(CHAR *sz)
{
    QWORD i;
    for (i = 0; sz[i] != 0; i++);
    return i;
}

QWORD strlenW(WCHAR *sz)
{
    QWORD i;
    for (i = 0; sz[i] != 0; i++);
    return i;
}

INT strcmpW(WCHAR *szA, WCHAR *szB)
{
    if (strlenW(szA) != strlenW(szB)) return 1;

    for (QWORD i = 0; i < szA[i]; i++)
        if (szA[i] != szB[i]) return 1;

    return 0;
}

#endif // __STRING_H
