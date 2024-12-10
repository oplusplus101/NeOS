
#ifndef __BOOTSTRUCTS_H
#define __BOOTSTRUCTS_H

typedef struct
{
    uint32_t nType;
    uint32_t nPad;
    size_t   nPhysicalStart;
    size_t   nVirtualStart; 
    uint64_t nNumberOfPages;
    uint64_t nAttribute;
} sEFIMemoryDescriptor;

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
    sEFIMemoryDescriptor *pMemoryDescriptor;
    size_t nMemoryMapSize;
    size_t nMemoryDescriptorSize;
    size_t nLoaderStart;
    size_t nLoaderEnd;
} __attribute__((packed)) sBootData;

#endif // __BOOTSTRUCTS_H
