
#ifndef __FS_H
#define __FS_H

#include <common/types.h>

typedef struct
{
    BYTE  arrJump[3];
    QWORD qwMagic;
    WORD  wBytesPerSector;
    BYTE  bSectorsPerCluster;
    QWORD qwNumberOfSectors;
    DWORD dwBootFileSystemSize;
    QWORD qwRootCluster; 
    BYTE  arrCode[476];
    WORD  wBootMagic;
} __attribute__((packed)) sBootSector;

typedef struct _tagBootFileDescriptor
{
    BYTE  bType; // 0 is empty, 1 is a file, 2 is a folder.
    WCHAR wsFileName[53];
    BYTE  bFileNameLength; // Filename length for faster enumeration
    QWORD qwFileSize;
    DWORD dwFirstCluster;
    QWORD qwWriteTimeStamp;
} __attribute__((packed)) sBootFileDescriptor;

typedef struct
{
    BYTE  nBootIndicator;
    DWORD nCHSStart : 24;
    BYTE  nOSType;
    DWORD nCHSEnd : 24;
    DWORD dwLBAStart;
    DWORD dwLBAEnd;
    BYTE  arrReserved[494];
    WORD  wMagicNumber;
} __attribute__((packed)) sGPTProtectiveMasterBootRecord;

typedef struct
{
    QWORD qwSignature;
    DWORD dwRevision;
    DWORD dwHeaderSize;
    DWORD dwHeaderCRC32;
    DWORD dwReserved0;
    QWORD qwCurrentLBA;
    QWORD qwBackupLBA;
    QWORD qwFirstUsableLBA;
    QWORD qwLastUsableLBA;
    QWORD qwGUIDLow;
    QWORD qwGUIDHigh;
    QWORD qwStartLBA;
    DWORD dwNumPartitions;
    DWORD dwPartititionSize;
    DWORD dwPartititionEntryCRC32;
    BYTE  arrReserved1[420];
} __attribute__((packed)) sGPTPartitionTableHeader;

typedef struct
{
    BYTE arrPartitionTypeGUID[16];
    BYTE arrUniquePartitionGUID[16];
    QWORD qwLBAStart;
    QWORD qwLBAEnd;
    QWORD qwAttributes;
    BYTE  sPartitionName[72];
} __attribute__((packed)) sGPTPartitionEntry;

typedef struct _tagRAMFile
{
    struct _tagRAMFile *pParent;
    BOOL  bFolder;
    WCHAR wsFileName[64];
    BYTE  bFileNameLength;
    QWORD qwFileSize;
    PVOID pData;
} sRAMFile;

void InitialiseFileSystem();
sRAMFile *GetBootFileSystem();
sRAMFile *GetFile(PWCHAR wszPath, sRAMFile *pParent);
sRAMFile *CollectFolderContents(sBootFileDescriptor *pFSFolder, sRAMFile *pParent);
QWORD GetBootFileSystemSize();

#endif // __FS_H
