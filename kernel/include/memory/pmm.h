
#ifndef __MEMORY__PMM_H
#define __MEMORY__PMM_H

#include <common/types.h>


void                  InitialisePhysicalMemoryManager(PBYTE pPageBitmap, QWORD qwBitmapLength);

PHYSICAL_ADDRESS SYSV PMMAllocatePage();
void                  PMMFreePage(PHYSICAL_ADDRESS Page);
PVOID                 PMMAllocatePagesContinuous(QWORD qwNumberOfPages);
PHYSICAL_ADDRESS      PMMAllocateContinuousPages(QWORD qwNumPages);
PHYSICAL_ADDRESS      PMMAllocateHugePage();
PHYSICAL_ADDRESS      PMMAllocateHugeContinuousPages(QWORD qwNumPages);


#endif // __MEMORY__PMM_H
