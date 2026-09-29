ENTRY(loader_main)

SECTIONS
{
    . = 0x10000;
    .text       : {
        *(.text.start)
        *(.text*)
    }
    .rodata     : { *(.rodata*) }
    .data       : { *(.data*) }
    
    . = ALIGN(4);
    __bss_start = .;
    .bss (NOLOAD) : { *(.bss*) *(COMMON) }
    __bss_end = .;

    /DISCARD/ : { *(.comment) *(.note*)
    *(.eh_frame*) }
}