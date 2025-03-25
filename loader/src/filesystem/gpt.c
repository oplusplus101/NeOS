
#include <filesystem/gpt.h>
#include <hardware/storage/drive.h>
#include <common/panic.h>
#include <common/memory.h>

static sGPTPartitionEntry g_arrPartitionEntries[128];
static sGPTPartitionEntry g_kernelPartition;

void LoadGPT(BYTE nDrive)
{
    sGPTProtectiveMasterBootRecord mbr;
    _ASSERT(ReadFromDrive(nDrive, 0, 1, &mbr), "GPT: Could not read LBA 0 from Drive #%d", nDrive);
    PrintString("GPT: LBA 0 loaded\n");
    sGPTPartitionTableHeader hdr;
    _ASSERT(ReadFromDrive(nDrive, 1, 1, &hdr), "GPT: Could not read LBA 1 from Drive #%d", nDrive);
    PrintString("GPT: LBA 1 loaded\n");
    _ASSERT(ReadFromDrive(nDrive, 2, 1, g_arrPartitionEntries), "GPT: Could not read LBA 2 from Drive #%d", nDrive);
    PrintString("GPT: LBA 2 loaded\n");
    
    if (hdr.qwSignature != 0x5452415020494645)
    _KERNEL_PANIC("Invalid GPT signature: 0x%16X", hdr.qwSignature);
    
    for (BYTE i = 0; i < 4; i++)
    {
        if (!memcmp(g_arrPartitionEntries[i].arrPartitionTypeGUID, (BYTE[16]) { 0 }, 16)) continue;
        if (!memcmp(g_arrPartitionEntries[i].arrPartitionTypeGUID, (BYTE[16]) { 0xA2, 0xA0, 0xD0, 0xEB, 0xE5, 0xB9, 0x33, 0x44, 0x87, 0xC0, 0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC7 }, 16))
        {
            memcpy(&g_kernelPartition, &g_arrPartitionEntries[i], sizeof(sGPTPartitionEntry));
            break;
        }
    }
}

sGPTPartitionEntry GetKernelPartition()
{
    return g_kernelPartition;
}
