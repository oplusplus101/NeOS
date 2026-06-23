
#ifndef __ARCH_H
#define __ARCH_H

#include <common/types.h>

typedef struct
{
    PVOID pBaseAddress;
    QWORD qwWidth, qwHeight;
    QWORD qwPitch;
    BYTE bBitsPerPixel;
    
    struct _tagDummyName
    {
        BYTE bRedStart;
        BYTE bGreenStart;
        BYTE bBlueStart;
        BYTE bRedLengthInBits;
        BYTE bGreenLengthInBits;
        BYTE bBlueLengthInBits;
    } sPixelFormat;
} sScreenInfo;

typedef struct
{
    DWORD dwType;
    DWORD dwPad;
    QWORD qwPhysicalStart;
    QWORD qwVirtualStart;
    QWORD qwNumberOfPages;
    QWORD qwAttribute;
} __attribute__((packed)) sEFIMemoryDescriptor;


typedef struct
{
    CHAR  sSignature[8];   // "RSD PTR "
    BYTE  bChecksum;
    CHAR  sOEMID[6];
    BYTE  bRevision;
    DWORD dwRSDTAddress;

    DWORD dwLength;
    QWORD qwXSDTAddress;
    BYTE  bExtendedChecksum;
    BYTE  arrReserved[3];
} __attribute__((packed)) sRSDP;

typedef struct
{
    CHAR  sSignature[4];
    DWORD dwLength;
    BYTE  bRevision;
    BYTE  bChecksum;
    CHAR  sOEMID[6];
    CHAR  sOEMTableID[8];
    DWORD dwOEMRevision;
    DWORD dwCreatorID;
    DWORD dwCreatorRevision;
} __attribute__((packed)) sSDTHeader;

typedef struct
{
    sSDTHeader sHeader;

    DWORD dwLocalAPICAddress;
    DWORD dwFlags;

    // variable entries follow
} __attribute__((packed)) sMADT;

typedef struct
{
    sScreenInfo sScreen;

    sEFIMemoryDescriptor *arrMemoryMap;
    QWORD qwMemoryMapSize, qwMemoryDescriptorSize;
    QWORD qwMemorySize;

    PBYTE pPageBitmap;
    QWORD qwPageBitmapSize;
    
    sRSDP *pRSDP;
    sSDTHeader *pXSDT;
    sMADT *pMADT;
    
    PVOID pPML4;

    PVOID pARCHPhysical;
    PVOID pKernelPhysical;
    QWORD qwARCHSize;
    QWORD qwKernelSize;

    PVOID pRoot;
    QWORD qwRootSize;

    QWORD qwKernelStackSize;
    QWORD qwKernelEntryRVA;
} sARCHHeader;


#endif // __ARCH_H
