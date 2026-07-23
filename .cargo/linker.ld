SECTIONS
{
    . = 0x0; 

    .text : {
        *(.text.boot)     
        *(.text .text.*)
    }

    .rodata : { *(.rodata .rodata.*) }
    .data   : { *(.data .data.*) }
    .bss    : { *(.bss .bss.*) }
}
