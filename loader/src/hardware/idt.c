
#include <hardware/idt.h>
#include <hardware/gdt.h>
#include <hardware/ports.h>
#include <common/screen.h>

sIDTEntry g_idt[256];
sIDTPointer g_idtr;
ESR g_ESRs[32];
ISR g_ISRs[224];

extern void HandleException0x00();
extern void HandleException0x01();
extern void HandleException0x02();
extern void HandleException0x03();
extern void HandleException0x04();
extern void HandleException0x05();
extern void HandleException0x06();
extern void HandleException0x07();
extern void HandleException0x08();
extern void HandleException0x09();
extern void HandleException0x0A();
extern void HandleException0x0B();
extern void HandleException0x0C();
extern void HandleException0x0D();
extern void HandleException0x0E();
extern void HandleException0x0F();
extern void HandleException0x10();
extern void HandleException0x11();
extern void HandleException0x12();
extern void HandleException0x13();
extern void HandleException0x14();
extern void HandleException0x15();
extern void HandleException0x16();
extern void HandleException0x17();
extern void HandleException0x18();
extern void HandleException0x19();
extern void HandleException0x1A();
extern void HandleException0x1B();
extern void HandleException0x1C();
extern void HandleException0x1D();
extern void HandleException0x1E();
extern void HandleException0x1F();

extern void HandleInterrupt0x20();
extern void HandleInterrupt0x21();
extern void HandleInterrupt0x22();
extern void HandleInterrupt0x23();
extern void HandleInterrupt0x81();
extern void IgnoreInterrupt();

void SetIDTEntry(BYTE nInterrupt, void (*pHandler)(), BYTE nFlags)
{
    sIDTEntry *pEntry = &g_idt[nInterrupt];
    pEntry->wISRLow   = ((QWORD) pHandler) & 0xFFFF;
    pEntry->nISRHigh  = ((QWORD) pHandler) >> 16;
    pEntry->nFlags    = nFlags;
    pEntry->wKernelCS = KERNEL_CODE_SEGMENT;
    pEntry->nIST      = 0;
    pEntry->dwReserved = 0;
}

void InitIDT()
{
    for (int i = 0; i < 256; i++)
    {
        SetIDTEntry(i, IgnoreInterrupt, 0x8E);
        if (i < 32) g_ESRs[i]      = 0;
        else        g_ISRs[i - 32] = 0;
    }
    
    SetIDTEntry(0x00, HandleException0x00, 0x8E);
    SetIDTEntry(0x01, HandleException0x01, 0x8E);
    SetIDTEntry(0x02, HandleException0x02, 0x8E);
    SetIDTEntry(0x03, HandleException0x03, 0x8E);
    SetIDTEntry(0x04, HandleException0x04, 0x8E);
    SetIDTEntry(0x05, HandleException0x05, 0x8E);
    SetIDTEntry(0x06, HandleException0x06, 0x8E);
    SetIDTEntry(0x07, HandleException0x07, 0x8E);
    SetIDTEntry(0x08, HandleException0x08, 0x8E);
    SetIDTEntry(0x09, HandleException0x09, 0x8E);
    SetIDTEntry(0x0A, HandleException0x0A, 0x8E);
    SetIDTEntry(0x0B, HandleException0x0B, 0x8E);
    SetIDTEntry(0x0C, HandleException0x0C, 0x8E);
    SetIDTEntry(0x0D, HandleException0x0D, 0x8E);
    SetIDTEntry(0x0E, HandleException0x0E, 0x8E);
    SetIDTEntry(0x0F, HandleException0x0F, 0x8E);
    SetIDTEntry(0x10, HandleException0x10, 0x8E);
    SetIDTEntry(0x11, HandleException0x11, 0x8E);
    SetIDTEntry(0x12, HandleException0x12, 0x8E);
    SetIDTEntry(0x13, HandleException0x13, 0x8E);
    SetIDTEntry(0x14, HandleException0x14, 0x8E);
    SetIDTEntry(0x15, HandleException0x15, 0x8E);
    SetIDTEntry(0x17, HandleException0x17, 0x8E);
    SetIDTEntry(0x18, HandleException0x18, 0x8E);
    SetIDTEntry(0x19, HandleException0x19, 0x8E);
    SetIDTEntry(0x1A, HandleException0x1A, 0x8E);
    SetIDTEntry(0x1B, HandleException0x1B, 0x8E);
    SetIDTEntry(0x1C, HandleException0x1C, 0x8E);
    SetIDTEntry(0x1D, HandleException0x1D, 0x8E);
    SetIDTEntry(0x1E, HandleException0x1E, 0x8E);
    SetIDTEntry(0x1F, HandleException0x1F, 0x8E);
    
    SetIDTEntry(0x20, HandleInterrupt0x20, 0x8E);
    SetIDTEntry(0x21, HandleInterrupt0x21, 0x8E);
    SetIDTEntry(0x22, HandleInterrupt0x22, 0x8E);
    SetIDTEntry(0x23, HandleInterrupt0x23, 0x8E);
    SetIDTEntry(0x81, HandleInterrupt0x81, 0x8E);
    
    // Initialize the PIC
    outb(PIC_MASTER_COMMAND, 0x11);
    IOWait();
    outb(PIC_SLAVE_COMMAND, 0x11);
    IOWait();
    // Master PIC starts at IRQ 0x20
    outb(PIC_MASTER_DATA, 0x20);
    IOWait();
    // Slave PIC starts at IRQ 0x28
    outb(PIC_SLAVE_DATA, 0x28);
    IOWait();
    outb(PIC_MASTER_DATA, 0x04);
    IOWait();
    outb(PIC_SLAVE_DATA, 0x02);
    IOWait();
    // Set the PICs to 8086 mode
    outb(PIC_MASTER_DATA, 0x01);
    IOWait();
    outb(PIC_SLAVE_DATA, 0x01);
    IOWait();
    // Set the masks
    outb(PIC_MASTER_DATA, 0x00);
    IOWait();
    outb(PIC_SLAVE_DATA, 0x00);
    IOWait();
    
    g_idtr.wLimit = sizeof(sIDTEntry) * 256 - 1;
    g_idtr.qwBase  = (QWORD) g_idt;
    __asm__ volatile ("lidt %0" : : "m" (g_idtr));
}

QWORD HandleInterrupt(BYTE nInterrupt, QWORD qwRSP, BYTE nErrorCode)
{
    if (nInterrupt < 32 && g_ESRs[nInterrupt] != 0)
        qwRSP = g_ESRs[nInterrupt](qwRSP, nErrorCode);
    else if (nInterrupt >= 32 && g_ISRs[nInterrupt - 32] != 0)
        qwRSP = g_ISRs[nInterrupt - 32](qwRSP);
    
    if (nInterrupt >= 0x28)
    {
        outb(PIC_SLAVE_COMMAND, 0x20);
        IOWait();
    }
    
    outb(PIC_MASTER_COMMAND, 0x20);
    IOWait();
    return qwRSP;
}


void RegisterException(BYTE n, ESR pESR)
{
    g_ESRs[n] = pESR;
}

void RegisterInterrupt(BYTE n, ISR pISR)
{
    g_ISRs[n - 32] = pISR;
}
