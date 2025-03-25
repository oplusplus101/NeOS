
#ifndef __BOOTSTRUCTS_H
#define __BOOTSTRUCTS_H

typedef struct
{
    unsigned int           nType;
    unsigned int           nPad;
    unsigned long long int nPhysicalStart;
    unsigned long long int nVirtualStart; 
    unsigned long long int nNumberOfPages;
    unsigned long long int nAttribute;
} sEFIMemoryDescriptor;

typedef struct
{
    void                  *pFramebuffer;
    unsigned long long int nBufferSize;
    unsigned int           nWidth;
    unsigned int           nHeight;
    unsigned int           nPixelsPerScanline;
} __attribute__((packed)) sGOPData;

typedef struct
{
    sGOPData               gop;
    sEFIMemoryDescriptor  *pMemoryDescriptor;
    unsigned long long int nMemoryMapSize;
    unsigned long long int nMemoryDescriptorSize;
    unsigned long long int nLoaderStart;
    unsigned long long int nLoaderEnd;
} __attribute__((packed)) sBootData;

#endif // __BOOTSTRUCTS_H
