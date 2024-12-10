
#ifndef __SCREEN_H
#define __SCREEN_H

#include <common/types.h>

typedef struct
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} __attribute__((packed)) color_t;

#define _RGB(r, g, b) ((color_t) { (r), (g), (b) })

void InitScreen(int nWidth, int nHeight, uint32_t *pScreenBuffer);
void DrawPixel(int x, int y, color_t c);
void PrintChar(char c);

void PrintString(const char *s);
void PrintDec(uint64_t n);
void PrintHex(uint64_t n, uint8_t nDigits, bool bUppercase);
void PrintFormat(const char *sFormat, ...);

void ClearScreen();
void SetCursor(int x, int y);
void SetFGColor(color_t c);
void SetBGColor(color_t c);

#endif // __SCREEN_H
