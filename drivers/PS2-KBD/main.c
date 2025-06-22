
#include <KNeOS.h>
#include <NeoPorts.h>

#define COMMAND_PORT 0x64
#define DATA_PORT    0x60

sModule *g_pHIDModule;
void (*g_funcKeyPressed)(BYTE);
void (*g_funcKeyReleased)(BYTE);
void (*g_funcUpdateKeyboardState)(BYTE);

void KeyboardInterrupt()
{
    BYTE bKey = inb(DATA_PORT);
    if (bKey & 0x80)
        g_funcKeyReleased(bKey);
    else
        g_funcKeyPressed(bKey);
}

void DriverMain()
{
    g_pHIDModule = KNeoGetModule("HID");
    g_funcKeyPressed = KNeoGetModuleFunction(g_pHIDModule, "_KeyPressed");
    g_funcKeyReleased = KNeoGetModuleFunction(g_pHIDModule, "_KeyReleased");
    
    // Disable interrupts so that the keyboard controller setup doesn't get messed up.
    KNeoDisableInterrupts();

    while (inb(COMMAND_PORT) & 0x01)
        inb(DATA_PORT);
    
    outb(COMMAND_PORT, 0xAE);
    outb(COMMAND_PORT, 0x20);
    BYTE bStatus = (inb(DATA_PORT) | 1) & ~0x10;
    outb(COMMAND_PORT, 0x60);
    outb(DATA_PORT, bStatus);
    outb(DATA_PORT, 0xF4);

    // 33 is the keyboard interrupt, ring 0
    KNeoRegisterInterrupt(33, 0, KeyboardInterrupt);
    // Re-enable interrupts so that other drivers can run too.
    KNeoEnableInterrupts();
    
    KNeoPauseProcess(KNeoGetCurrentPID());
}
