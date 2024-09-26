
#ifndef __BOOTHEADER_H
#define __BOOTHEADER_H


typedef struct
{
    void *pFramebuffer;
    unsigned long long int nBufferSize;
    unsigned int nWidth;
    unsigned int nHeight;
    unsigned int nPixelsPerScanline;
} __attribute__((packed)) sGopData;

typedef struct
{
    sGopData gop;
} __attribute__((packed)) sBootData;

#endif // __BOOTHEADER_H
