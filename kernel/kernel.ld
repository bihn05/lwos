OUTPUT_FORMAT("elf64-x86-64")
ENTRY(start)

KERNEL_VMA = 0xFFFFFFFF80000000;
KERNEL_LMA = 0x00200000;

SECTIONS
{
    . = KERNEL_VMA;

    _kernel_start = .;

    .text ALIGN(0x1000) : AT(KERNEL_LMA) {
        build/kernel/start.o(.text)
        *(.text)
    }

    .rodata ALIGN(0x1000) : AT(KERNEL_LMA + (ADDR(.rodata) - KERNEL_VMA)) {
        *(.rodata*)
    }

    .data ALIGN(0x1000) : AT(KERNEL_LMA + (ADDR(.data) - KERNEL_VMA)) {
        *(.data*)
    }

    .bss ALIGN(0x1000) : AT(KERNEL_LMA + (ADDR(.bss) - KERNEL_VMA)) {
        *(.bss*)
        *(COMMON)
    }

    _kernel_end = ALIGN(0x1000);

    /DISCARD/ : {
        *(.comment)
        *(.eh_frame)
    }
}