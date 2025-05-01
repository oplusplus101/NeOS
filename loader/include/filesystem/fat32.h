
#ifndef __FAT32_H
#define __FAT32_H

#include <filesystem/gpt.h>


#define FAT32_ATTR_READ_ONLY 0x01
#define FAT32_ATTR_HIDDEN    0x02
#define FAT32_ATTR_SYSTEM    0x04
#define FAT32_ATTR_VOLUME_ID 0x08
#define FAT32_ATTR_DIRECTORY 0x10
#define FAT32_ATTR_ARCHIVE   0x20
#define FAT32_ATTR_LFN       (FAT32_ATTR_READ_ONLY | FAT32_ATTR_HIDDEN | FAT32_ATTR_SYSTEM | FAT32_ATTR_VOLUME_ID)

typedef struct
{
    BYTE nHour : 5;
    BYTE nMinute : 6;
    BYTE nTimeTwoSeconds : 5;
} __attribute__((packed)) sFAT32Time;

typedef struct
{
    BYTE nYear : 7;
    BYTE nMonth : 4;
    BYTE nDay : 5;
} __attribute((packed)) sFAT32Date;

typedef struct
{
    sFAT32Time sTime;
    sFAT32Date sDate;
} __attribute__((packed)) sFAT32Timestamp;

typedef struct
{
    BYTE  arrJumpCode[3];
    BYTE  sOEMIdentifier[8];
    WORD  wBytesPerSector;
    BYTE  nSectorsPerCluster;
    WORD  wNumReservedSectors;
    BYTE  nNumFATs;
    WORD  wNumRootDirectoryEntries;
    WORD  wTotalSectors16;
    BYTE  nMediaType;
    WORD  wNumSectorsPerFAT;
    WORD  wNumSectorsPerTrack;
    WORD  wNumHeads;
    DWORD dwNumHiddenSectors;
    DWORD dwTotalSectors32;

    // FAT32 Extended part
    DWORD dwSectorsPerFAT;
    WORD  wFlags;
    WORD  wFATVersionNumber;
    DWORD dwRootDirectoryCluster;
    WORD  wFSInfoSectorNumber;
    WORD  wBackupBootSectorNumber;
    BYTE  arrReserved[12];
    BYTE  nDriveNumber;
    BYTE  nWinNTFlags;
    BYTE  nSignature;
    DWORD dwVolumeSerialNumber;
    BYTE  sVolumeID[11];
    BYTE  sSystemIDString[8];
    BYTE  arrBootCode[420];
    WORD  wMagicNumber;
} __attribute__((packed)) sFAT32BootSector;

typedef struct
{
    BYTE            sFilename[11];
    BYTE            nAttributes;
    BYTE            nReservedNT;
    BYTE            nCreationTimeOneHundredths;
    sFAT32Timestamp sCreationTimestamp;
    sFAT32Date      sAccessDate;
    WORD            nClusterHigh;
    sFAT32Timestamp sModificationTimestamp;
    WORD            nClusterLow;
    DWORD           dwFileSize;
} __attribute__((packed)) sFAT32DirectoryEntry;

typedef struct
{
    BYTE nSequence;
    WORD sFirstPart[5];
    BYTE nAttribute;
    BYTE nEntryType;
    BYTE nChecksum;
    WORD sSecondPart[6];
    WORD wReserved;
    WORD sThirdPart[2];
} __attribute__((packed)) sFAT32LongFilenameEntry;

BOOL GetEntryFromPath(PWCHAR wszPath, sFAT32DirectoryEntry *pEntry);
void ReadDirectoryEntry(sFAT32DirectoryEntry *pEntry, PVOID pBuffer);
void LoadFAT32(BYTE nDrive, sGPTPartitionEntry kernelPartition);

#endif // __FAT32_H
