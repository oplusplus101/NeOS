
#ifndef __NTFS_H
#define __NTFS_H

#define MFT_INDEX_$MFT      0
#define MFT_INDEX_$MFT_MIRR 1
#define MFT_INDEX_$LOG_FILE 2
#define MFT_INDEX_$VOLUME   3
#define MFT_INDEX_$ATTRDEF  4
#define MFT_INDEX_$ROOT     5
#define MFT_INDEX_$BADCLUS  6
#define MFT_INDEX_$QUOTA    7

typedef LONGLONG LCN; // Logical Cluster Number
typedef LONGLONG VCN; // Virtual Cluster Number
typedef LONGLONG LSN; // Log Sequence Number

typedef struct
{
    BYTE  arrJumpCode[3];
    BYTE  arrSignature[8];
    WORD  wBytesPerSector;
    BYTE  nSectorsPerCluster;
    WORD  wReservedSectors;
    BYTE  nNumFileAllocationTables;
    WORD  wRootDirectoryEntries;
    WORD  wTotalSectors16;
    BYTE  nMediaDescriptor;
    WORD  nSectorsPerFAT;
    WORD  wSectorsPerTrack;
    WORD  wNumHeads;
    QWORD qwNumHiddenSectors;
    QWORD qwTotalSectors32;
    BYTE  nDiscUnit;
    BYTE  nFlags;
    BYTE  nBPBVersionSignature;
    BYTE  nReserved;
    QWORD qwTotalSectors64;
    LCN   lcnMasterFileTable;
    LCN   lcnMasterFileTableMirror;
    QWORD qwMasterFileTableClusterBlock;
    BYTE  nMasterFileTableEntrySize;
    BYTE  arrReserved[3];
    QWORD qwNTFSVolumeSerial;
    DWORD dwChecksum;
    BYTE  arrBootCode[426];
    WORD  wBootSignature; // Shoud be 0xAA55
} __attribute__((packed)) sNTFSVolumeHeader;

typedef struct
{
    // Multi Sector Header
    DWORD dwSignature; // Shoud be 0x454C4946 (ASCII 'FILE')
    WORD  wUpdateSequenceArrayOffset;
    WORD  wUpdateSequenceArraySize;
    LSN   lsnLogSequenceNumber;
    WORD  wSequence;
    WORD  wReference;
    WORD  wFirstAttributeOffset;
    WORD  wEntryFlags;
    DWORD dwUsedEntrySize;
    DWORD dwTotalEntrySize;
    QWORD qwBaseRecordFileReference;
    WORD  wFirstAvailableAttributeIdentifier;
    WORD  wFixupPattern;
} __attribute__((packed)) sNTFSFileRecordSegmentHeader;

#endif // __NTFS_H
