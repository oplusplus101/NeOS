
ENTRY(LoaderMain);

SECTIONS
{
    . = 0x100000;

    g_nLoaderStart = .;

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

    g_nLoaderEnd = .;
}
