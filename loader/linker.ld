
ENTRY(LoaderMain);
OUTPUT_ARCH(i386:x86-64);
OUTPUT_FORMAT(pei-x86-64);

SECTIONS
{
    . = 0x100000;

    .text : ALIGN(4096)
    {
        *(.text)
    }

    .data : ALIGN(4096)
    {
        *(.data)
    }

    .rodata : ALIGN(4096)
    {
        *(.rodata)
    }

    .bss : ALIGN(4096)
    {
        *(COMMON)
        *(.bss)
    }

    /DISCARD/ :
    {
        *(.comment)
    }
}
