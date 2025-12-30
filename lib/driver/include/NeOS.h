
#ifndef __NEOS_H
#define __NEOS_H

typedef struct
{
    DWORD dwWidth, dwHeight;
} sSize;

typedef struct
{
    INT dwWidth, dwHeight;
} sPoint;

typedef struct
{
    sPoint pt;
    sSize sz;
} sRectangle;

typedef struct
{
    sRectangle rectArea;
    BYTE       nColorsPerPixel;
    BYTE       nBytesPerPixel;
    DWORD      dwRefreshRate;
} sNeoScreenInfo;

#endif // __NEOS_H
