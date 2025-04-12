
#include <common/string.h>
#include <common/screen.h>

PCHAR g_szStrtokNext = NULL;

PCHAR strtok(PCHAR sz, const PCHAR szDelim)
{
    if (sz != NULL)
        g_szStrtokNext = sz;
    QWORD qwSZLen    = strlen(g_szStrtokNext);
    QWORD qwDelimLen = strlen(szDelim);

    if (qwSZLen < qwDelimLen) return NULL;

    for (QWORD i = 0; i <= qwSZLen - qwDelimLen; i++)
    {
        if (!strncmp(g_szStrtokNext + i, szDelim, qwDelimLen))
        {
            g_szStrtokNext[i] = 0;
            g_szStrtokNext += i + qwDelimLen;
            return g_szStrtokNext - i - qwDelimLen;
        }
    }
    
    return NULL;
}
