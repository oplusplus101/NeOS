
#include <common/countedstring.h>
#include <common/memory.h>
#include <common/string.h>
#include <memory/heap.h>

sString MakeString(const PWCHAR wsz)
{
    sString ws;
    ws.qwLength = strlenW(wsz);
    ws.wszData  = KHeapAlloc((ws.qwLength + 1) * sizeof(PWCHAR));
    strncpyW(ws.wszData, wsz, ws.qwLength * sizeof(PWCHAR));
    return ws;
}

void DestroyString(sString *ws)
{
    KHeapFree(ws->wszData);
    ws->qwLength = 0;
}

PWCHAR CopyStringAsCString(sString *ws)
{
    PWCHAR wsz = KHeapAlloc((ws->qwLength + 1) * sizeof(PWCHAR));
    if (wsz == NULL || ws->wszData == NULL)
        return NULL;
    strncpyW(wsz, ws->wszData, ws->qwLength);
    return wsz;
}

sString CopyString(sString *ws)
{
    sString wsCopy;
    wsCopy.qwLength = ws->qwLength;
    wsCopy.wszData  = KHeapAlloc((ws->qwLength + 1) * sizeof(PWCHAR));
    strncpyW(wsCopy.wszData, ws->wszData, wsCopy.qwLength);
    return wsCopy;
}

sString *AppendString(sString *pwsA, sString *pwsB)
{
    pwsA->wszData = KHeapReAlloc(pwsA->wszData, (pwsA->qwLength + pwsB->qwLength + 1) * sizeof(PWCHAR));
    strncpyW(pwsA->wszData + pwsA->qwLength, pwsB->wszData, pwsB->qwLength);
    pwsA->qwLength += pwsB->qwLength;
    return pwsA;
}


sString StringFromInt(LONGLONG ll, BYTE bBase)
{
    if (ll == 0)
        return MakeString(L"0");

    INT j = 0;
    if (ll < 0)
    {
        j++;
        ll = -ll;
    }
    
    PCHAR sDigits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    CHAR sBuffer[20];
    QWORD nDigits = 0;
    while (ll)
    {
        sBuffer[nDigits++] = sDigits[ll % bBase];
        ll /= bBase;
    }

    sString ws;
    ws.qwLength = nDigits;
    ws.wszData  = KHeapAlloc((nDigits + j + 1) * sizeof(PWCHAR));
    if (j != 0)
        ws.wszData[0] = '-';

    for (INT i = nDigits - 1; i >= 0; i--, j++)
        ws.wszData[j] = sBuffer[i];
    
    ws.wszData[nDigits + j] = 0;
    
    return ws;
}

sString StringFromUInt(QWORD qw, BYTE bBase)
{
    if (qw == 0)
        return MakeString(L"0");

    PCHAR sDigits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    CHAR sBuffer[20];
    QWORD nDigits = 0;
    while (qw)
    {
        sBuffer[nDigits++] = sDigits[qw % bBase];
        qw /= bBase;
    }

    sString ws;
    ws.qwLength = nDigits;
    ws.wszData  = KHeapAlloc((nDigits + 1) * sizeof(PWCHAR));

    for (INT i = nDigits - 1, j = 0; i >= 0; i--, j++)
        ws.wszData[j] = sBuffer[i];

    ws.wszData[nDigits] = 0;

    return ws;
}

