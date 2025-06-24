
#include <runtime/driver.h>
#include <runtime/process.h>

sList g_lstDrivers;

void InitDrivers()
{
    g_lstDrivers = CreateEmptyList(sizeof(sDriver));
}

sDriver *GetDriver(PWCHAR wszName)
{
    for (QWORD i = 0; i < g_lstDrivers.qwLength; i++)
    {
        sDriver *pDriver = GetListElement(&g_lstDrivers, i);
        if (!strcmpW(pDriver->wszName, wszName))
            return pDriver;
    }

    return NULL;
}

