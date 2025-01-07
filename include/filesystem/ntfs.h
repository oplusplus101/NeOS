
#ifndef __NTFS_H
#define __NTFS_H

#include <common/types.h>

typedef struct
{
    BYTE arrJMPInstruction[3];
    BYTE arrOEMSystemName[8];
    WORD wBytesPerSector;
    BYTE nSectorsPerCluster;
    WORD wReservedSectorCount;
    BYTE nTableCount;
    WORD wRootEntryCount;
    WORD wSectorCount;
    BYTE nMediaType;
    WORD wSectorsPerTable;
    WORD wSectorsPerTrack;
    WORD wHeadCount;
    DWORD dwHiddenSectorCount;
    DWORD dwSectorCount;
    DWORD dwReseved;
    QWORD qwSectorCount;
} __attribute__((packed)) sNTFSBootSector;

typedef struct
{
    QWORD qwMasterFileCluster;
    QWORD qwMasterFileTableMirrorCluster;
    BYTE  nClustersPerRecord;
    BYTE  arrReserved0[3];
    BYTE  nClustersPerIndexBuffer;
    BYTE  arrReserved1[3];
    QWORD qwSerialNumber;
    WORD  wChecksum;
} __attribute__((packed)) sNTFSHeader;

#endif // __NTFS_H
