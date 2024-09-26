
#include <common/types.h>
#include <common/bootstructs.h>
#include <common/math.h>

void KernelMain(sBootData hdr)
{
    // Pattern from: https://youtu.be/hxOw_p0kLfI?t=42
//    __asm__ volatile ("cli\nhlt");
    for (int y = 0; y < hdr.gop.nHeight; y++)
    {
        for (int x = 0; x < hdr.gop.nWidth; x++)
        {
            int l = min(0x1FF >> min(min(min(min(x, y), hdr.gop.nWidth - 1 - x), hdr.gop.nHeight - 1 - y), 31u), 255);
            int d = 50;
            ((uint32_t *) hdr.gop.pFramebuffer)[x + y * hdr.gop.nWidth] = 65536 * min(max((int) ((~x & ~y) & 0xFF) - d, l), 255) +
                                                                          256   * min(max((int) (( x & ~y) & 0xFF) - d, l), 255) +
                                                                                  min(max((int) ((~x &  y) & 0xFF) - d, l), 255);
        }
    }
}
