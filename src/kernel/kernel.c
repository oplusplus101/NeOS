
#include <common/types.h>
#include <common/bootheaders.h>

void _start(BootHeader hdr)
{
    int x = 0;
    while (1)
    {
        for (int j = 0; j < hdr.gop.nHeight; j++)
            for (int i = 0; i < hdr.gop.nWidth; i++)
            {
                ((uint32_t *) hdr.gop.pFramebuffer)[i + j * hdr.gop.nWidth] = (j + x) & 0xFF;
            }
        x++;
    }
}
