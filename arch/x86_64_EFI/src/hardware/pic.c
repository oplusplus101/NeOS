
#include <hardware/pic.h>
#include <hardware/ports.h>

void _InitialisePIC(sARCHHeader *pHeader)
{
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
}
