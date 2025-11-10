
#ifndef __LOADERFUNCTIONS_H
#define __LOADERFUNCTIONS_H

#include <common/types.h>
#include <neos.h>

#define LOG_LOG 0
#define LOG_WARNING 1
#define LOG_ERROR 2
#define LOG_GOODBYE 3

// Returns a handle to the file, or NULL if not found
extern PVOID (*LoaderOpenFile)(PWCHAR wszFileName);
// Returns the file size in bytes
extern QWORD (*LoaderGetFileSize)(PVOID pFile);
// Will read no more bytes than specified by qwSize, will return the total number of bytes read .
extern QWORD (*LoaderReadFile)(PVOID pFile, PVOID pBuffer, QWORD qwSize);
// Frees the file handle
extern void  (*LoaderCloseFile)(PVOID pFile);
extern void  (*LoaderSeekFile)(PVOID, QWORD);
extern QWORD (*LoaderTellFile)(PVOID);


extern void (*PrintFormat)(const PWCHAR wszFormat, ...);
extern void (*PrintString)(PCHAR szString);
extern void (*PrintBytes)(PVOID pBuffer, QWORD qwLength, WORD wBytesPerLine, BOOL bASCII);
extern void (*PrintChar)(CHAR c);
extern void (*Log)(INT iType, const PWCHAR wszFormat, ...);
extern INT  (*GetCursorX)();
extern INT  (*GetCursorY)();
extern void (*SetCursor)(INT x, INT y);
extern void (*SetFGColor)(color_t c);
extern void (*SetBGColor)(color_t c);
extern void (*ClearScreen)();
extern INT  (*GetScreenWidth)();
extern INT  (*GetScreenHeight)();

void InitialiseLoaderFunctions(sNEOSKernelHeader *pHeader);

#endif // __LOADERFUNCTIONS_H
