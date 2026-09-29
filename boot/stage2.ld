ENTRY(stage2_entry)

STACK_SIZE = 16K;
STUB_SECTS = 8;

SECTIONS
{
    . = 0x900;

    .stage2.entry : { *(.stage2.entry) }

    __stub_end = .;

    ASSERT(__stub_end <= 0x900 + STUB_SECTS * 512,
            "STAGE2 EXCEEDS ITS SECTOR BUDGET")
    ASSERT(0x900 + STUB_SECTS * 512 <= 0x7c00,
            "STAGE2 OVERRUNS 7C00H")
            
    /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) }
}