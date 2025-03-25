
#ifndef __ATAPI_H
#define __ATAPI_H

#include <common/types.h>

#define DATA 0
#define ERROR_R 1
#define SECTOR_COUNT 2
#define LBA_LOW 3
#define LBA_MID 4
#define LBA_HIGH 5
#define DRIVE_SELECT 6
#define COMMAND_REGISTER 7
#define ALTERNATE_STATUS 0
#define CONTROL 0x206

BOOL ReadCDROMPIO(WORD nPort, BOOL bSlave, DWORD dwLBA, DWORD dwSectors, WORD *pBuffer);

#endif // __ATAPI_H
