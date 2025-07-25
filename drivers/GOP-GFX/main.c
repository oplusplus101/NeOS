
#include <NeoTypes.h>
#include <KNeOS.h>

DWORD g_dwWidth, g_dwHeight;
QWORD g_qwFramebufferSize;
PVOID g_pFramebuffer, g_pDriver;

FUNC_EXPORT void KNeoSetGOPData(PVOID pFramebuffer, QWORD qwFramebufferSize, DWORD dwWidth, DWORD dwHeight)
{
    g_pFramebuffer      = pFramebuffer;
    g_qwFramebufferSize = qwFramebufferSize;
    g_dwWidth           = dwWidth;
    g_dwHeight          = dwHeight;
}

// The color format is 0x00BBGGRR
FUNC_EXPORT void NeoSetPixel(DWORD x, DWORD y, DWORD dwColor)
{
    if (x >= g_dwWidth || y >= g_dwHeight) return;
    *((PBYTE) g_pFramebuffer + (x * g_dwWidth + y) * 3)     = dwColor;
    *((PBYTE) g_pFramebuffer + (x * g_dwWidth + y) * 3 + 1) = dwColor >> 8;
    *((PBYTE) g_pFramebuffer + (x * g_dwWidth + y) * 3 + 2) = dwColor >> 16;
}

FUNC_EXPORT DWORD NeoGetPixel(DWORD x, DWORD y)
{
    if (x >= g_dwWidth || y >= g_dwHeight) return 0;
    return *((PDWORD) ((PBYTE) g_pFramebuffer + (x * g_dwWidth + y) * 3)) & 0x00FFFFFF;
}

FUNC_EXPORT PVOID KNeoGetFrameBuffer()
{
    return g_pFramebuffer;
}

void DriverMain()
{
    g_pDriver = KNeoGetDriver("GFX");
    _ASSERT(g_pDriver != NULL, "Graphics Driver (GFX.mod) not found");
    // ((void (*)(PCHAR)) KNeoGetDriverFunction(g_pDriver, "NeoRegisterScreenDriver"))("GOP-GFX");

    KNeoPauseProcess(KNeoGetCurrentPID());
}
