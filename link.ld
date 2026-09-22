ENTRY(stage2_entry)

STACK_SIZE = 16K;
STUB_SECTS = 8;

KERNEL_LOAD_PHYS = 0x10000;

SECTIONS
{
    . = 0x900;

    .stage2.entry : { *(.stage2.entry) }

    __stub_end = .;

    . = 0x100000;

    .kernel :
    {
        __kernel_vma = .;

        __abi_start = .;
        *(.abi)
        __abi_end = .;

        *(.text) *(.text.*)
        *(.rodata) *(.rodata.*)
        *(.data) *(.data.*)
        . = ALIGN(4);
        __kernel_vma_end = .;
    }

    .bss (NOLOAD) :
    {
        __bss_start = .;
        *(.bss) *(.bss.*) *(COMMON)
        . = ALIGN(16);
        . = . + STACK_SIZE;
        __stack_top = .;
        __bss_end = .;
    }

    __kernel_size = __kernel_vma_end - __kernel_vma;
/*
    ASSERT(__abi_start == 0x100000,
            "ABI TABLE IS NOT AT 1M, SOMETHING GOT LINKED IN FRONT OF .abi")
    ASSERT(__abi_end > __abi_start,
            "ABI TABLE IS EMPTY, abi.o MISSING FROM THE LINK")*/
    ASSERT(__stub_end <= 0x900 + STUB_SECTS * 512,
            "REAL MODE STUB EXCEEDS STUB_SECTS SECTORS, RAISE IT HERE AND STAGE2_SECTS IN boot/mbr.s")
    ASSERT(0x900 + STUB_SECTS * 512 <= 0x7c00,
            "STUB_SECTS OVERRUNS THE MBR AT 7C00H, WHICH IS LIVE DURING ITS OWN READ")
    ASSERT(KERNEL_LOAD_PHYS + __kernel_size <= 0xA0000,
            "KERNEL DOESNT FIT BELOW A0000H WHEN LAODED IN REAL MODE")

    /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) }
}