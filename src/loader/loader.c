
#include <common/types.h>
#include <common/bootstructs.h>
#include <common/screen.h>
#include <hardware/gdt.h>

void LoaderMain(sBootData data)
{
    InitGDT();
    LoadGDT();
    InitScreen(data.gop.nWidth, data.gop.nHeight, data.gop.pFramebuffer);
    ClearScreen();
    SetBGColor(RGB(0, 0, 0));
    SetFGColor(RGB(168, 168, 168));
    PrintString("Hello, World!");

    while (1);
}
