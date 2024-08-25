
#ifndef __BOOTHEADER_H
#define __BOOTHEADER_H


typedef struct
{
    void *pFramebuffer;
    unsigned long long int nBufferSize;
    unsigned int nWidth;
    unsigned int nHeight;
    unsigned int nPixelsPerScanline;
} GopData;

typedef struct
{
    GopData gop;
} BootHeader;

#endif // __BOOTHEADER_H
