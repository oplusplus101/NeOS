
#ifndef __COMMON__STRING_H
#define __COMMON__STRING_H

#include <common/types.h>
#include <memory/heap.h>

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

inline static void strcpyW(PWCHAR wszDest, PWCHAR wszSrc)
{
    for (QWORD i = 0; *wszSrc; i++)
        *(wszDest++) = *(wszSrc++);
    *wszDest = 0;
}

inline static void strncpy(PCHAR szDest, PCHAR szSrc, QWORD n)
{
    for (QWORD i = 0; *szSrc && i < n; i++)
        *(szDest++) = *(szSrc++);
    *szDest = 0;
}

inline static void strncpyW(PWCHAR wszDest, PWCHAR wszSrc, QWORD n)
{
    for (QWORD i = 0; *wszSrc && i < n; i++)
        *(wszDest++) = *(wszSrc++);
    *wszDest = 0;
}

inline static PCHAR strcat(PCHAR szDest, PCHAR sz)
{
    strcpy(&szDest[strlen(szDest)], sz);
    return szDest;
}

inline static PWCHAR strcatW(PWCHAR wszDest, PWCHAR wsz)
{
    strcpyW(&wszDest[strlenW(wszDest)], wsz);
    return wszDest;
}

inline static INT strncmp(PCHAR szA, PCHAR szB, QWORD n)
{
    for (QWORD i = 0; (szA[i] || szB[i]) && i < n; i++)
        if (szA[i] != szB[i]) return 1;

    return 0;
}

inline static INT strncmpW(PWCHAR wszA, PWCHAR wszB, QWORD n)
{
    for (QWORD i = 0; (wszA[i] || wszB[i]) && i < n; i++)
        if (wszA[i] != wszB[i]) return 1;

    return 0;
}

inline static INT strcmpW(PWCHAR szA, PWCHAR szB)
{
    for (QWORD i = 0; szA[i] || szB[i]; i++)
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

inline static PCHAR strdup(PCHAR sz)
{
    PCHAR szNew = KHeapAlloc(strlen(sz) + 1); // add 1 for the null terminator
    strcpy(szNew, sz);
    return szNew;
}

inline static PWCHAR strdupW(PWCHAR sz)
{
    PWCHAR szNew = KHeapAlloc((strlenW(sz) + 1) * sizeof(WCHAR)); // add 1 for the null terminator
    strcpyW(szNew, sz);
    return szNew;
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

inline static void ToUppercase(PCHAR sz)
{
    for (; *sz != 0; sz++)
        if (*sz >= 'a' && *sz <= 'z')
            *sz -= 'a' - 'A';
}

inline static void ToUppercaseW(PWCHAR wsz)
{
    for (; *wsz != 0; wsz++)
        if (*wsz >= L'a' && *wsz <= L'z')
            *wsz -= L'a' - L'A';
}

PCHAR strtok(PCHAR sz, const PCHAR szDelim);
PWCHAR strtokW(PWCHAR wsz, const PWCHAR wszDelim);

#endif // __COMMON__STRING_H
