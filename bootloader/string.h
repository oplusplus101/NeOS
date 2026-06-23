
#ifndef __STRING_H
#define __STRING_H


inline static QWORD strlenW(PWCHAR wsz)
{
    QWORD i;
    for (i = 0; wsz[i] != 0; i++);
    return i;
}

PWCHAR strtokW(PWCHAR wsz, const PWCHAR wszDelim)
{
    static PWCHAR g_wszStrtokNext = NULL;
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

#endif // __STRING_H
