
#include <KNeOS.h>

// This module implements the interface between software and the filesystem driver.
// To ensure that multiple filesystems are supported, the software interface is split from the filesystem driver.
// For example, if the system used the FAT-32 filesystem, and a USB-stick is plugged in that uses the exFAT filesystem, the same function calls can be used.

enum
{
    FM_BINARY = 0b0000,
    FM_TEXT   = 0b0001,
    FM_READ   = 0b0010,
    FM_WRITE  = 0b0100,
    FM_APPEND = 0b1000
};

typedef struct
{
    INT nMode;
    PCHAR szFilename;
} sFile;

sDriver *g_pDriver;

// Can either be a drive letter (A-Z) or a mount point, if the filesystem containing the supports it
INT NeoGetFilesystemType(PCHAR szDrivePath)
{

}

sFile *NeoOpenFile(PCHAR szFilename, BYTE nMode)
{
    sFile sFile = NeoHeapAlloc();
}

void NeoCloseFile(sFile *pFile)
{
    ((void (*)(sFile *)) KNeoGetDriverFunction(g_pDriver, "CloseFile"))(pFile);
}


void NeoSeekFile(QWORD qwOffset, CHAR nMode)
{
    
}

void NeoReadFile(sFile *pFile, PVOID pBuffer, QWORD qwSize)
{

}

void NeoWriteFile(sFile *pFile, PVOID pBuffer, QWORD qwSize)
{
    
}

void ModuleEnable()
{
    g_pDriver = KNeoLoadDriver(KNeoGetRegistry("NEOS/DEFAULTS/FILESYSTEM_DRIVER"));
}

void ModuleDisable()
{
    
}
