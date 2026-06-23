
#ifndef __HARDWARE__APIC_H
#define __HARDWARE__APIC_H

#ifdef ARCH_EXE
#include <arch.h>
void _InitialiseAPIC(sARCHHeader *pHeader);
#endif

void ArchEndOfInterrupt(BYTE bInterrupt);

#endif // __HARDWARE__APIC_H
