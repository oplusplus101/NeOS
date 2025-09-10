
#include <KNeOS.h>
#include <NeoPorts.h>

#define PS2_COMMAND_PORT  0x64
#define PS2_DATA_PORT     0x60

#define PS2_READ_CONFIG   0x20
#define PS2_WRITE_CONFIG  0x60
#define PS2_SELF_TEST     0xAA
#define PS2_TEST_PORT1    0xAB
#define PS2_TEST_PORT2    0xA9
#define PS2_ENABLE_PORT1  0xAE
#define PS2_ENABLE_PORT2  0xA8
#define PS2_DISABLE_PORT1 0xAD
#define PS2_DISABLE_PORT2 0xA7

#define KBD_ENABLE_SCAN   0xF4
#define KBD_ACK           0xFA
#define KBD_RESEND        0xFE

BOOL g_bHasSecondPort;
// sDriver *g_pHIDDriver;
void (*g_funcKeyPressed)(BYTE);
void (*g_funcKeyReleased)(BYTE);
void (*g_funcUpdateKeyboardState)(BYTE);

void WriteCommand(BYTE c)
{
    outb(PS2_COMMAND_PORT, c);
}

void WriteData(BYTE d)
{
    outb(PS2_DATA_PORT, d);
}

BYTE ReadCommand()
{
    return inb(PS2_COMMAND_PORT);
}

BYTE ReadData()
{
    return inb(PS2_DATA_PORT);
}

BYTE ReadConfig()
{
    WriteCommand(PS2_READ_CONFIG);
    return ReadData();
}

void WriteConfig(BYTE b)
{
    WriteCommand(PS2_WRITE_CONFIG);
    WriteData(b);
}

QWORD KeyboardInterrupt(QWORD qwRSP)
{
    BYTE bKey = ReadData();
    // __asm__ volatile ("int $0x81" :: "a"(0x60));
    CHAR test[2] = " ";
    test[0] = bKey;
    KNeoPrintString(test);
    // if (bKey & 0x80)
        // g_funcKeyReleased(bKey);
    // else
        // g_funcKeyPressed(bKey);

    return qwRSP;
}

INT DriverMain(sDriverObject *pObject)
{
    // g_pHIDDriver = KNeoGetDriver("HID");
    // g_funcKeyPressed = KNeoGetDriverFunction(g_pHIDDriver, "_KeyPressed");
    // g_funcKeyReleased = KNeoGetDriverFunction(g_pHIDDriver, "_KeyReleased");
    
    // TODO: Check if the controller exists

    // Disable the ports
    WriteCommand(PS2_DISABLE_PORT1);
    WriteCommand(PS2_DISABLE_PORT2);

    // Flush the output buffer
    while (ReadCommand() & 0x01)
        ReadData();

    // Change the configuration byte
    BYTE bConfig = ReadConfig();
    bConfig &= ~0b01010001; // Clear bits 0, 4, 6
    WriteConfig(bConfig);

    // Do a self-test
    WriteCommand(PS2_SELF_TEST);
    if (ReadData() != 0x55)
    {
        KNeoPrintString("PS2: Controller self-test failed!\n");
        WriteCommand(PS2_DISABLE_PORT1);
        WriteCommand(PS2_DISABLE_PORT2);
        return 1;
    }
    
    // Restore the configuration byte (compatibility stuff for some controllers)
    WriteCommand(PS2_WRITE_CONFIG);
    WriteData(bConfig);

    // Enable the second port.
    WriteCommand(PS2_ENABLE_PORT2);
    WriteCommand(PS2_READ_CONFIG);
    g_bHasSecondPort = false;
    
    // Check if the second port exists
    if (!(ReadData() & 0x20))
    {
        g_bHasSecondPort = true;
        // Disable the second port.
        WriteCommand(PS2_DISABLE_PORT2);
        WriteCommand(PS2_READ_CONFIG);
        BYTE bConfig = ReadData();
        bConfig &= ~0b00100001;
    }

    // Test the interface
    WriteCommand(PS2_TEST_PORT1);
    ReadData();
    if (g_bHasSecondPort)
    {
        WriteCommand(PS2_TEST_PORT2);
        ReadData();
    }

    // Enable the first port
    WriteCommand(PS2_ENABLE_PORT1);
    if (g_bHasSecondPort)
        WriteCommand(PS2_ENABLE_PORT2);
    
    WriteCommand(KBD_ENABLE_SCAN);


    // WriteCommand(PS2_DISABLE_PORT1);
    // while(ReadCommand() & 0x01)
    //     ReadData();
    // WriteCommand(PS2_ENABLE_PORT1); // activate interrupts
    // BYTE status = (ReadConfig() | 1) & ~0x10;
    // WriteConfig(status);
    
    
    // Finally register the interrupt
    KNeoRegisterInterrupt(0x21, 0, KNeoGetPhysicalAddress(KeyboardInterrupt));
    
    return 0;
}
