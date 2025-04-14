
#ifndef __STRING_H
#define __STRING_H

#include <common/types.h>

inline static QWORD strlen(PCHAR sz)
{
    QWORD i;
    for (i = 0; sz[i] != 0; i++);
    return i;
}

inline static QWORD strlenW(PWCHAR sz)
{
    QWORD i;
    for (i = 0; sz[i] != 0; i++);
    return i;
}

inline static INT strcmp(PCHAR szA, PCHAR szB)
{
    if (strlen(szA) != strlen(szB)) return 1;

    for (QWORD i = 0; szA[i] && szB[i]; i++)
        if (szA[i] != szB[i]) return 1;

    return 0;
}

inline static void strcpy(PCHAR szDest, PCHAR szSrc)
{
    for (QWORD i = 0; *szSrc; i++)
        *(szDest++) = *(szSrc++);
    *szDest = 0;
}

inline static void strncpy(PCHAR szDest, PCHAR szSrc, QWORD n)
{
    for (QWORD i = 0; *szSrc && i < n; i++)
        *(szDest++) = *(szSrc++);
    *szDest = 0;
}

inline static INT strncmp(PCHAR szA, PCHAR szB, QWORD n)
{
    for (QWORD i = 0; szA[i] && szB[i] && i < n; i++)
        if (szA[i] != szB[i]) return 1;

    return 0;
}

inline static INT strcmpW(PWCHAR szA, PWCHAR szB)
{
    if (strlenW(szA) != strlenW(szB)) return 1;

    for (QWORD i = 0; szA[i] && szB[i]; i++)
        if (szA[i] != szB[i]) return 1;

    return 0;
}


inline static BOOL ContainsChar(PCHAR sz, CHAR c)
{
    for (; *sz != 0; sz++)
    if (*sz == c) return true;
    
    return false;
}

inline static BOOL iswhitespace(CHAR c)
{
    return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

inline static BOOL isdigit(CHAR c)
{
    return c >= '0' && c <= '9';
}

inline static BOOL isalpha(CHAR c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

inline static BOOL isalnum(CHAR c)
{
    return isdigit(c) || isalpha(c);
}

inline static PCHAR StripString(PCHAR sz)
{
    // First remove the prefix
    QWORD qwPrefix = 0;
    while (iswhitespace(sz[qwPrefix++]) && sz[qwPrefix] != 0);
    qwPrefix--;
    if (qwPrefix != 0) strcpy(sz, sz + qwPrefix);

    // Then the suffix
    QWORD qwSuffix = strlen(sz) - 1;
    while (qwSuffix != 0 && iswhitespace(sz[qwSuffix--]));
    sz[qwSuffix + 2] = 0;
    return sz;
}


PCHAR strtok(PCHAR sz, const PCHAR szDelim);

#endif // __STRING_H
