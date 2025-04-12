
#ifndef __NEOFS_H
#define __NEOFS_H

#include "types.h"

/*
GOAL:
Create an FAT-32 or NTFS-like filesystem that supports compression, encryption, case-sensitive filenames up to 256 characters.
The filesystem will also have permissions (files that can only be accessed by certain users and/or permission levels).
There will also be a type of file called a "virtual file", which only exists for certain programs/processes and is invisible to others.
The GPT GUID for this filesystem will be 1cc5dee3-bd03-44d1-ac11-1836aaa6bae1

Ownership:
Files belong to users and groups
The user and group id 0xFFFF are used for internal files, while user and group id 0x0000 is the system user/group.

Time:
The epoch will be on the 25st of December in the year 4 BC at 00:00
To convert form UNIX time, just subtract 62262001254.

Folders:
Folders will essentially be a filesystem within a filesystem, i.e. it will contain File Table Entries.
This would make encryption slightly more secure as individual folders can have their own keys.

Data format:
The data, if longer than one cluster will be have an LCN at the beginning of each cluster in order to indicate the next cluster.
If the LCN is 0, there are no more clusters left.

Permissions:
The permissions are similar to UNIX - Read, Write, and Execute.
There are ?? levels:
0 - Nobody
1 - Owner
2 - Owner and Group
3 - Everyone

Layout:
LBA 0 - 1            Boot Sector
LBA 1 - Cluster Size Padding
LBA 2 - N            Master File Table
LBA N+               Data
*/

typedef QWORD LCN; // Logical Cluster Number

#define NEOFS_ATTR_FILESYSTEM 1 // Internal file (e.g. bitmaps, logs)
#define NEOFS_ATTR_READONLY   2
#define NEOFS_ATTR_HIDDEN     4 // File will not be display by a viewer (by default)
#define NEOFS_ATTR_VIRTUAL    8 // File will only exist for a certain program/process
#define NEOFS_ATTR_FOLDER     16
#define NEOFS_ATTR_MOUNTPOINT 32
#define NEOFS_ATTR_SYMLINK    64
#define NEOFS_ATTR_ENCRYPTED  128
#define NEOFS_ATTR_COMPRESSED 256

// FAT-like bootsector (not backwards compatible)
typedef struct
{
    BYTE  arrJumpCode[3];
    BYTE  sSignature[8];
    WORD  wBytesPerSector;
    BYTE  nSectorsPerCluster;
    BYTE  nMediaType;
    QWORD qwSectorCount;
    WORD  wNumRootEntries;
    BYTE  arrBootCode[485];
    WORD  wMagicNumber;
} __attribute__((packed)) sNEOFSBootSector;

typedef struct
{
    WCHAR    sName[256];
    BYTE     nFilenameLength; // This should make searching slightly faster as a filename that doesn't match the length of the target can just be skipped
    BYTE     nAttributes;
    BYTE     nPermissions;
    WORD     wOwner;
    WORD     wGroup;
    LONGLONG llModifyTime;
    LONGLONG llAccessTime;
    LONGLONG llCreateTime;
    QWORD    qwFileSize;
    LCN      lcnFirstCluster;
    BYTE     arrPadding[465];
} __attribute__((packed)) sNEOFSDirectoryEntry;


#endif // __NEOFS_H
