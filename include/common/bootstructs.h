
#ifndef __BOOTHEADER_H
#define __BOOTHEADER_H

typedef struct
{
    uint32_t nType;
    uint32_t nPad;
    size_t   nPhysicalStart;
    size_t   nVirtualStart;
    uint64_t nNumberOfPages;
    uint64_t nAttribute;
} __attribute__((packed)) sEFIMemoryDescriptor;

typedef struct
{
    void *pFramebuffer;
    size_t nBufferSize;
    uint32_t nWidth;
    uint32_t nHeight;
    uint32_t nPixelsPerScanline;
} __attribute__((packed)) sGopData;

typedef struct
{
    sGopData gop;
    sEFIMemoryDescriptor memoryDescriptor;
} __attribute__((packed)) sBootData;

#endif // __BOOTHEADER_H
