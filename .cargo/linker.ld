ENTRY(_start)

PHDRS
{
    text PT_LOAD FLAGS(5);
    data PT_LOAD FLAGS(6);
}

SECTIONS
{
    . = 0x100000;

    .text ALIGN(16) :
    {
        KEEP(*(.text.boot))
        *(.text*)
    } :text

    .rodata ALIGN(16) :
    {
        *(.rodata .rodata.*)
    } :text

    .data ALIGN(16) :
    {
        *(.data .data.*)
    }

    .bss ALIGN(16) (NOLOAD) :
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
