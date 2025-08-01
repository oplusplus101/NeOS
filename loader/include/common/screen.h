
#ifndef __COMMON__SCREEN_H
#define __COMMON__SCREEN_H

#include <common/types.h>

typedef struct
{
    BYTE r, g, b;
} __attribute__((packed)) color_t;

#define _RGB(r, g, b) ((color_t) { (r), (g), (b) })

void InitScreen(int nWidth, int nHeight, DWORD *pScreenBuffer);
void DrawPixel(int x, int y, color_t c);
void PrintChar(char c);

void PrintString(const PCHAR sz);
void PrintStringW(const PWCHAR wsz);
void PrintDec(QWORD qw);
void PrintHex(QWORD qw, BYTE nDigits, BOOL bUppercase);
void PrintFormat(const PCHAR sFormat, ...);
void PrintBytes(PVOID pBuffer, QWORD qwLength, WORD wBytesPerLine, BOOL bASCII);
void FillRectangle(int x, int y, int w, int h, color_t c);

INT GetScreenWidth();
INT GetScreenHeight();

void ClearScreen();
void SetCursor(int x, int y);
int GetCursorX();
int GetCursorY();
void SetFGColor(color_t c);
void SetBGColor(color_t c);

#endif // __COMMON__SCREEN_H
