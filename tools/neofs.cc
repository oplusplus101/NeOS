
#include <filesystem>
#include <vector>
#include <iostream>
#include <fstream>
#include <cstring>
#include "neofs.h"

struct sDirectoryTemplate
{
    WCHAR    szName[256];
    BYTE     nFilenameLength; // This should make searching slightly faster
    BYTE     nAttributes;
    BYTE     nPermissions;
    WORD     wOwner;
    WORD     wGroup;
    LONGLONG llModifyTime;
    LONGLONG llAccessTime;
    LONGLONG llCreateTime;
    QWORD    qwFileSize;
    LCN      lcnFirstCluster;
    std::string sOriginalPath;
    std::vector<sDirectoryTemplate> vecSubentries;
};


void IterateDirectory(const std::filesystem::directory_entry &rootEntry, sDirectoryTemplate *pParent)
{
    for (const auto &entry : std::filesystem::directory_iterator(rootEntry.path().string()))
    {
        std::string sPath = entry.path().string();
        QWORD qwFilenameOffset = sPath.find_last_of("/\\");
        std::string sFilename;
        if (qwFilenameOffset == 18446744073709551615UL)
            sFilename = sPath;
        else
            sFilename = sPath.substr(qwFilenameOffset + 1);

        sDirectoryTemplate current;
        memset(current.szName, 0, 256);
        for (QWORD i = 0; i < sPath.length(); i++)
            current.szName[i] = sPath[i];
        current.nFilenameLength = sPath.length();
        current.sOriginalPath = entry.path().string();
        LONGLONG llTime = time(nullptr) - 62262001254; // Convert to custom time format
            
        current.nAttributes  = entry.is_directory() ? NEOFS_ATTR_FOLDER : 0;
        current.nPermissions = 0b111111;
        current.wOwner       = 0;
        current.wGroup       = 0;
        current.llModifyTime = llTime;
        current.llAccessTime = llTime;
        current.llCreateTime = llTime;
        
        if (entry.is_directory())
            IterateDirectory(entry, &current);
        pParent->vecSubentries.push_back(current);
        std::cout << "Adding entry \"" << sFilename << "\" with path \"" << entry.path().string() << "\"" << std::endl;
    }
};


int main(int nszArgc, char **arrArgv)
{
    if (nszArgc != 3)
    {
        std::cout << "Usage: " << arrArgv[0] << " <folder> <out file>" << std::endl;
        return 1;
    }

    std::ifstream in(arrArgv[2], std::ios::binary);
    in.seekg(0, std::ios::end);
    QWORD qwSize = in.tellg(); 
    in.close();

    std::ofstream strOutput(arrArgv[2], std::ios::binary | std::ios::trunc);

    sNEOFSBootSector bs = { 0 };
    memcpy(bs.sSignature, "NEOFS   ", 8);
    bs.wBytesPerSector    = 512;
    bs.nSectorsPerCluster = 4;
    bs.nMediaType         = 0x80;
    bs.qwSectorCount      = qwSize / (QWORD) bs.wBytesPerSector;
    bs.wNumRootEntries    = 16; // There can be max. 256 files in the root
    bs.wMagicNumber       = 0xAA55;
    strOutput.write((PCHAR) &bs, 512);
    QWORD qwBytesPerCluster = bs.nSectorsPerCluster * bs.wBytesPerSector;
    if (qwBytesPerCluster > 512)
    {
        PBYTE pBuffer = new BYTE[qwBytesPerCluster - 512];
        strOutput.write((PCHAR) pBuffer, qwBytesPerCluster);
    }

    std::cout << "Initialising disk with " << qwSize << " bytes" << std::endl; 
    
    std::vector<sDirectoryTemplate> vecEntriesRoot;
    QWORD qwBitmapSize = bs.qwSectorCount / bs.nSectorsPerCluster / 8 + 1;
    BYTE *arrBitmap = new BYTE[qwBitmapSize];
    QWORD qwBitmapIndex = ((QWORD) bs.wNumRootEntries * 1024) / qwBytesPerCluster + 1;
    memset(arrBitmap, 0xFF, qwBitmapIndex / 8);

    auto GetCluster = [&]()
    {
        for (; qwBitmapIndex < bs.qwSectorCount / (QWORD) bs.nSectorsPerCluster; qwBitmapIndex++)
        {
            std::cout << "Cluster #" << qwBitmapIndex << std::endl;
            if (arrBitmap[qwBitmapIndex / 8] & (1 << (qwBitmapIndex % 8))) continue;
            arrBitmap[qwBitmapIndex / 8] |= 1 << (qwBitmapIndex % 8);
            
            return (LCN) qwBitmapIndex;
        }
        
        std::cout << "Not enough space" << std::endl;
        exit(1);
        return (LCN) 0UL;
    };

    auto AddRootDirectoryEntry = [&](const std::string &sFilename, const std::string &sOriginalPath, BYTE nAttributes, BYTE nPermissions, WORD wOwner, WORD wGroup)
    {
        sDirectoryTemplate entry;
        memset(entry.szName, 0, 256);
        for (QWORD i = 0; i < sFilename.length(); i++)
            entry.szName[i] = sFilename[i];
        entry.nFilenameLength = sFilename.length();
        
        LONGLONG llTime = time(nullptr) - 62262001254; // Convert to custom time format
        entry.sOriginalPath = sOriginalPath;
        entry.nAttributes  = nAttributes;
        entry.nPermissions = nPermissions;
        entry.wOwner       = wOwner;
        entry.wGroup       = wGroup;
        entry.llModifyTime = llTime;
        entry.llAccessTime = llTime;
        entry.llCreateTime = llTime;
        vecEntriesRoot.push_back(entry);
        return vecEntriesRoot.size() - 1;
    };

    auto AddData = [&](LCN lcn, PVOID pData, QWORD qwLength)
    {
        while (qwLength > 0)
        {
            std::cout << "Cluster " << lcn << " " << qwLength << " bytes left" << std::endl;
            LCN lcnNext;
            if (qwLength > qwBytesPerCluster - 8)
                lcnNext = GetCluster();
            else
                lcnNext = 0;
            QWORD qwWriteSize = lcnNext != 0 ? qwBytesPerCluster - 8 : qwLength;
            strOutput.seekp(lcn * qwBytesPerCluster);
            strOutput.write((CHAR *) &lcnNext, 8);
            strOutput.write((PCHAR) pData, qwWriteSize);
            lcn = lcnNext;
            qwLength -= qwWriteSize;
            pData = (BYTE *) pData + qwWriteSize;
        }
    };
    
    auto AddDataStream = [&](LCN lcn, std::ifstream &ifs)
    {
        PBYTE pBuffer = new BYTE[qwBytesPerCluster - 8];
        QWORD qwTotalWritten = 0;
        while (true)
        {
            std::cout << "Cluster " << lcn << std::endl;
            QWORD qwRead = ifs.readsome((PCHAR) pBuffer, qwBytesPerCluster - 8);
            LCN lcnNext;
            if (qwRead == qwBytesPerCluster - 8)
                lcnNext = GetCluster();
            else
                lcnNext = 0;

            strOutput.seekp(lcn * qwBytesPerCluster);
            strOutput.write((PCHAR) &lcnNext, 8);
            strOutput.write((PCHAR) pBuffer, qwRead);
            lcn = lcnNext;
            qwTotalWritten += qwRead;
            if (lcnNext == 0) break;
        }
        delete[] pBuffer;
        return qwTotalWritten;
    };
    
    // Setup internal files
    // Master File Table
    AddRootDirectoryEntry("$MFT", "", NEOFS_ATTR_FILESYSTEM, 0b111111, 0xFFFF, 0xFFFF);
    vecEntriesRoot[0].qwFileSize = (QWORD) bs.wNumRootEntries * 1024;
    vecEntriesRoot[0].lcnFirstCluster = 1;
    // Master File Table Mirror at the end of the disk in reversed order (not yet implemented)
    AddRootDirectoryEntry("$MFTMIRROR", "", NEOFS_ATTR_FILESYSTEM, 0b111111, 0xFFFF, 0xFFFF);
    // Shows which clusters are reserved/free
    AddRootDirectoryEntry("$BITMAP", "", NEOFS_ATTR_FILESYSTEM, 0b111111, 0xFFFF, 0xFFFF);
    // Lists changes made to the filesystem (i.e reads, writes)
    AddRootDirectoryEntry("$LOG", "", NEOFS_ATTR_FILESYSTEM, 0b111111, 0xFFFF, 0xFFFF);

    // Setup $BITMAP
    // vecEntriesRoot[2].qwFileSize      = qwBitmapSize;
    // vecEntriesRoot[2].lcnFirstCluster = GetCluster();

    // AddData(vecEntriesRoot[2].lcnFirstCluster, arrBitmap, qwBitmapSize);
    
    // Search the root directory
    for (const auto &entry : std::filesystem::directory_iterator(arrArgv[1]))
    {
        std::string sPath = entry.path().string().substr(strlen(arrArgv[1]));

        WORD wIndex = AddRootDirectoryEntry(sPath, entry.path().string(), entry.is_directory() ? NEOFS_ATTR_FOLDER : 0, 0b111111, 0x0000, 0x0000);

        if (entry.is_directory()) // Very dumb idea
            IterateDirectory(entry, &vecEntriesRoot[wIndex]);
        std::cout << "Adding root entry \"" << sPath << "\"" << std::endl;
    }

    for (auto &entry : vecEntriesRoot)
    {
        sNEOFSDirectoryEntry dirent;
        memcpy(dirent.szName, entry.szName, 256);
        dirent.nFilenameLength = entry.nFilenameLength; // This should make searching slightly faster
        dirent.nAttributes     = entry.nAttributes;
        dirent.nPermissions    = entry.nPermissions;
        dirent.wOwner          = entry.wOwner;
        dirent.wGroup          = entry.wGroup;
        dirent.llModifyTime    = entry.llModifyTime;
        dirent.llAccessTime    = entry.llAccessTime;
        dirent.llCreateTime    = entry.llCreateTime;
        dirent.qwFileSize = 0;
        dirent.lcnFirstCluster = GetCluster();
        if (!(dirent.nAttributes & (NEOFS_ATTR_FOLDER | NEOFS_ATTR_FILESYSTEM)))
        {
            std::ifstream ifs(entry.sOriginalPath, std::ios::binary);
            QWORD qwWritten = AddDataStream(dirent.lcnFirstCluster, ifs);
            std::cout << "Wrote " << qwWritten << " bytes to file \"" << entry.sOriginalPath << "\"" << std::endl;
        }
        strOutput.write((CHAR *) &dirent, 1024);
    }

    strOutput.close();
    delete[] arrBitmap;
    return 0;
}
