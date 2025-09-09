
#include <KNeOS.h>

void Thread()
{
    for (;;)
    {
        HANDLE h;
        INT iStatus = KNeoCreateFile(&h, L"Devices\\TEST2", 0, 0, 0, 0, 0);
        KNeoPrintFormat(L"Status: %d\n", iStatus);
        for (int i = 0; i < 0x8FFFFFF; i++) __asm__ volatile ("nop");
    }
}

INT DriverMain(sDriverObject *pObject)
{
    // KNeoStartProcess(Thread, 1024 * 4096);
    for (int i = 0;; i++)
    {
        KNeoPrintFormat(L"DRM: %d\n", i);
        HANDLE h;
        INT iStatus = KNeoCreateFile(&h, L"Devices\\TEST2", 0, 0, 0, 0, 0);
        KNeoPrintFormat(L"Status: %d\n", iStatus);
        for (int i = 0; i < 0x8FFFFFF; i++) __asm__ volatile ("nop");
    }
    return 0;
}
