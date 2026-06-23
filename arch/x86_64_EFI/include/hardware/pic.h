
#ifndef __PIC_H
#define __PIC_H

#include <arch.h>

#define PIC_MASTER_COMMAND 0x20
#define PIC_SLAVE_COMMAND  0xA0

#define PIC_MASTER_DATA    0x21
#define PIC_SLAVE_DATA     0xA1

void _InitialisePIC(sARCHHeader *pHeader);

#endif // __PIC_H
