
#include <loaderfunctions.h>


PVOID (*LoaderOpenFile)(PWCHAR wszFileName);
QWORD (*LoaderGetFileSize)(PVOID pFile);
QWORD (*LoaderReadFile)(PVOID pFile, PVOID pBuffer, QWORD qwSize);
void  (*LoaderCloseFile)(PVOID pFile);
void  (*LoaderSeekFile)(PVOID pFile, QWORD qwPosition);
QWORD (*LoaderTellFile)(PVOID pFile);

void (*PrintFormat)(const PWCHAR wszFormat, ...);
void (*PrintString)(PCHAR szString);
void (*PrintBytes)(PVOID pBuffer, QWORD qwLength, WORD wBytesPerLine, BOOL bASCII);
void (*PrintChar)(CHAR c);
void (*Log)(INT iType, const PWCHAR wszFormat, ...);
INT  (*GetCursorX)();
INT  (*GetCursorY)();
void (*SetCursor)(INT x, INT y);
void (*SetFGColor)(color_t c);
void (*SetBGColor)(color_t c);
void (*ClearScreen)();
INT  (*GetScreenWidth)();
INT  (*GetScreenHeight)();

void InitialiseLoaderFunctions(sNEOSKernelHeader *pHeader)
{
    LoaderOpenFile    = pHeader->LoaderOpenFile;
    LoaderGetFileSize = pHeader->LoaderGetFileSize;
    LoaderReadFile    = pHeader->LoaderReadFile;
    LoaderCloseFile   = pHeader->LoaderCloseFile;
    LoaderSeekFile    = pHeader->LoaderSeekFile;
    LoaderTellFile    = pHeader->LoaderTellFile;

    PrintFormat     = pHeader->PrintFormat;
    PrintString     = pHeader->PrintString;
    PrintBytes      = pHeader->PrintBytes;
    PrintChar       = pHeader->PrintChar;
    Log             = pHeader->Log;
    GetCursorX      = pHeader->GetCursorX;
    GetCursorY      = pHeader->GetCursorY;
    SetCursor       = pHeader->SetCursor;
    SetFGColor      = pHeader->SetFGColor;
    SetBGColor      = pHeader->SetBGColor;
    ClearScreen     = pHeader->ClearScreen;
    GetScreenWidth  = pHeader->GetScreenWidth;
    GetScreenHeight = pHeader->GetScreenHeight;
}
