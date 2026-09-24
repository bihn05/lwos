ENTRY(monitor_main)

STACK_SIZE = 16K;

SECTIONS
{
    . = 0x100000;

    __abi_start = .;
    .abi : { *(.abi) }
    __abi_end = .;

    .text : {
        *(.text.start)
        *(.text*)
    }

    .rodata : { *(.rodata*) }
    .data   : { *(.data*) }

    . = ALIGN(16);
    __bss_start = .;
    .bss (NOLOAD) : { *(.bss*) *(COMMON) }
    . = ALIGN(16);
    . = . + STACK_SIZE;
    __stack_top = .;
    __bss_end = .;

    ASSERT(__abi_start == 0x100000, "ABI 表没有落在 0x100000")
    ASSERT(__abi_end > __abi_start, "ABI 表是空的，abi.o 没链进来")

    /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) }

}