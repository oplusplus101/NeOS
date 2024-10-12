
#include <common/bootstructs.h>

void LoaderMain(sBootData data)
{
    int q = 311;
    for (int i = 0; i < data.gop.nWidth * data.gop.nHeight * 4; i++)
    {
        ((unsigned int *) data.gop.pFramebuffer)[i] = i;
    }
}
