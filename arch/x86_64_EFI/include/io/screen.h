
#ifndef __IO__SCREEN_H
#define __IO__SCREEN_H

#include <common/types.h>
#include <arch.h>
#include <ramfile.h>

void _InitialiseScreen(sScreenInfo sInfo, sRAMFile *pFont);

void SYSV ArchClearScreen();

void SYSV ArchSetFGColour(sColour c);
void SYSV ArchSetBGColour(sColour c);

void SYSV ArchPrintChar(WCHAR c);
void MS ArchPrintFormat(const PWCHAR wszFormat, ...);
void SYSV ArchPrintFormatVariadic(const PWCHAR wszFormat, __builtin_va_list lst);

INT  SYSV ArchGetCursorX();
INT  SYSV ArchGetCursorY();
void SYSV ArchSetCursor(INT x, INT y);

INT  SYSV ArchScreenWidth();
INT  SYSV ArchScreenHeight();

INT  SYSV ArchTextWidth();
INT  SYSV ArchTextHeight();

void SYSV ArchDrawLine(INT x0, INT y0, INT x1, INT y1, sColour c);
void SYSV ArchPutPixel(INT x, INT y, sColour c);

#endif // __IO__SCREEN_H
