
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

    KNeoPrintFormat(L"Value: %d\n", bKey);
    
    return qwRSP;
}

INT DriverMain(sDriverObject *pObject)
{
    // g_pHIDDriver = KNeoGetDriver("HID");
    // g_funcKeyPressed = KNeoGetDriverFunction(g_pHIDDriver, "_KeyPressed");
    // g_funcKeyReleased = KNeoGetDriverFunction(g_pHIDDriver, "_KeyReleased");
    
    // TODO: Check if the controller exists

    while (ReadCommand() & 1)
        ReadData();
    
    WriteCommand(0xAE); // activate interrupts
    WriteCommand(0x20); // command 0x20 = read controller command byte
    BYTE bStatus = (ReadData() | 1) & ~0x10;
    WriteCommand(0x60); // command 0x60 = set controller command byte
    WriteData(bStatus);
    WriteData(0xF4);

    // Set the device up
    sDeviceObject *pDeviceObject;
    INT iStatus = KNeoCreateDevice(pObject, L"Devices\\PS2", 0, &pDeviceObject);

    if (iStatus != 0)
        KNeoLog(L"Failed to create the device '\\Devices\\PS2'; The PS2 keyboard and mouse will not work!", 2);
    
    // Finally register the interrupt
    // KNeoRegisterInterrupt(0x21, 0, (void (*)())(KeyboardInterrupt));
    // KNeoRegisterInterrupt(0x21, 0, KNeoGetPhysicalAddress(KeyboardInterrupt));

    return 0;
}
