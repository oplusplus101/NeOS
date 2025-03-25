
ENTRY(KernelMain);
OUTPUT_ARCH(i386:x86-64);
OUTPUT_FORMAT(elf64-x86-64);

SECTIONS
{
    . = 0x200000;

    .text :
    {
        *(.text)
    }

    .data :
    {
        *(.data)
    }

    .rodata :
    {
        *(.rodata)
    }

    .bss :
    {
        *(COMMON)
        *(.bss)
    }

    /DISCARD/ :
    {
        *(.comment)
    }
}
