
#ifndef __COMMON__SCREEN_H
#define __COMMON__SCREEN_H

#include <common/types.h>
#include <neos.h>

void InitScreen(int nWidth, int nHeight, DWORD *pScreenBuffer);
sColour GetPixel(INT x, INT y);
void DrawPixel(int x, int y, sColour c);
void PrintChar(char c);

void PrintString(const PCHAR sz);
void PrintStringW(const PWCHAR wsz);
void PrintDec(QWORD qw);
void PrintHex(QWORD qw, BYTE nDigits, BOOL bUppercase);
void PrintFormatVariadic(const PWCHAR wszFormat, __builtin_va_list args);
void PrintFormat(const PWCHAR wszFormat, ...);
void PrintBytes(PVOID pBuffer, QWORD qwLength, WORD wBytesPerLine, BOOL bASCII);
void FillRectangle(int x, int y, int w, int h, sColour c);

INT GetScreenWidth();
INT GetScreenHeight();

void ClearScreen();
void SetCursor(int x, int y);
int GetCursorX();
int GetCursorY();
void SetFGColor(sColour c);
void SetBGColor(sColour c);

#endif // __COMMON__SCREEN_H
