OUTPUT_FORMAT("elf64-x86-64")
ENTRY(start)

SECTIONS
{
    . = 0x10000;

    .text : {
        /* 强制 start.o 的代码排在 0x10000 的起始位置 */
        build/kernel/start.o(.text)
        *(.text)
    }

    .rodata : { *(.rodata*) }
    .data   : { *(.data*) }
    
    .bss : {
        *(.bss*)
        *(COMMON)
    }

    /DISCARD/ : {
        *(.comment)
        *(.eh_frame)
    }
}