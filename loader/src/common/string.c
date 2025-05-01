
#include <common/string.h>
#include <common/screen.h>

PCHAR g_szStrtokNext = NULL;
PWCHAR g_wszStrtokNext = NULL;

PCHAR strtok(PCHAR sz, const PCHAR szDelim)
{
    if (sz != NULL) g_szStrtokNext = sz;
    if (g_szStrtokNext == NULL) return NULL;
    QWORD qwLength = strlen(g_szStrtokNext);

    for (QWORD i = 0; i <= qwLength; i++)
        for (QWORD j = 0; szDelim[j]; j++)
        {
            if (g_szStrtokNext[i] != szDelim[j] && g_szStrtokNext[i] != 0) continue;
            if (g_szStrtokNext[i] == 0)
            {
                PCHAR szBackup = g_szStrtokNext;
                g_szStrtokNext = NULL;
                return szBackup;
            }
            g_szStrtokNext[i] = 0;
            g_szStrtokNext += i + 1;
            return g_szStrtokNext - i - 1;
        }
    
    return NULL;
}

PWCHAR strtokW(PWCHAR wsz, const PWCHAR wszDelim)
{
    if (wsz != NULL) g_wszStrtokNext = wsz;
    if (g_wszStrtokNext == NULL) return NULL;
    QWORD qwLength = strlenW(g_wszStrtokNext);

    for (QWORD i = 0; i <= qwLength; i++)
        for (QWORD j = 0; wszDelim[j]; j++)
        {
            if (g_wszStrtokNext[i] != wszDelim[j] && g_wszStrtokNext[i] != 0) continue;
            if (g_wszStrtokNext[i] == 0)
            {
                PWCHAR wszBackup = g_wszStrtokNext;
                g_wszStrtokNext = NULL;
                return wszBackup;
            }
            g_wszStrtokNext[i] = 0;
            g_wszStrtokNext += i + 1;
            return g_wszStrtokNext - i - 1;
        }
    
    return NULL;
}
