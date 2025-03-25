
#include <hardware/storage/atapi.h>
#include <hardware/ports.h>

// This code is to wait 400 ns
static void ATAIOWait(const BYTE p) {
	inb(p + CONTROL + ALTERNATE_STATUS);
	inb(p + CONTROL + ALTERNATE_STATUS);
	inb(p + CONTROL + ALTERNATE_STATUS);
	inb(p + CONTROL + ALTERNATE_STATUS);
}

// Reads sectors starting from lba to buffer
BOOL ReadCDROMPIO(WORD nPort, BOOL bSlave, DWORD nLBA, DWORD nSectors, WORD *pBuffer)
{
	volatile BYTE arrReadCommand[12] = { 0xA8, 0,
	                                    (nLBA >> 0x18) & 0xFF, (nLBA >> 0x10) & 0xFF, (nLBA >> 0x08) & 0xFF,
	                                    (nLBA >> 0x00) & 0xFF,
	                                    (nSectors >> 0x18) & 0xFF, (nSectors >> 0x10) & 0xFF, (nSectors >> 0x08) & 0xFF,
	                                    (nSectors >> 0x00) & 0xFF,
	                                    0, 0 };

	outb(nPort + DRIVE_SELECT, 0xA0 & (bSlave << 4)); // Drive select
	ATAIOWait(nPort);
	outb(nPort + ERROR_R, 0x00); 
	outb(nPort + LBA_MID, 2048 & 0xFF);
	outb(nPort + LBA_HIGH, 2048 >> 8);
	outb(nPort + COMMAND_REGISTER, 0xA0); // Packet command
	ATAIOWait(nPort); // I think we might need this delay, not sure, so keep this
 
    // Wait for status
	while (true)
    {
		BYTE nStatus = inb(nPort + COMMAND_REGISTER);
		if ((nStatus & 0x01) == 1)
			return false;
		if (!(nStatus & 0x80) && nStatus & 0x08)
			break;
		ATAIOWait(nPort);
	}

    // Send command
	outsw(nPort + DATA, (WORD *) arrReadCommand, 6);

    // Read words
	for (DWORD i = 0; i < nSectors; i++)
    {
        // Wait until ready
		while (true)
        {
			BYTE nStatus = inb(nPort + COMMAND_REGISTER);
			if (nStatus & 0x01)
				return false;
			if (!(nStatus & 0x80) && (nStatus & 0x08))
				break;
		}

		int size = inb(nPort + LBA_HIGH) << 8
		           | inb(nPort + LBA_MID); // Get the size of transfer

		insw(nPort + DATA, (WORD *) ((BYTE *) pBuffer + i * 0x800), size / 2); // Read it
	}

	return true;
}
