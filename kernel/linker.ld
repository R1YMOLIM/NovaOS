ENTRY(_start)

PHDRS
{
    text PT_LOAD FLAGS(5);
    data PT_LOAD FLAGS(6);
}

SECTIONS
{
    . = 0x100000;

    .text ALIGN(4096) :
    {
        KEEP(*(.text.main))
        *(.text*)
    } :text

    .rodata ALIGN(4096) :
    {
        *(.rodata .rodata.*)
    } :text

    .data ALIGN(4096) :
    {
        *(.data .data.*)
    }

    .bss ALIGN(4096) (NOLOAD) :
    {
        *(.bss .bss.*)
        *(COMMON)
    }

    /DISCARD/ :
    {
        *(.eh_frame*)
        *(.eh_frame_hdr*)
        *(.comment*)
        *(.note*)
    }
}
