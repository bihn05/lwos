
#include "abi.h"
#include "ctx.h"

PVOID *lw_abi_base;

/* ABI 不可用时连 puts 都没有, 只能直接写显存 */
static void no_abi_halt(void) {
    const char *msg = "NO ABI.BIN AT 100000H";
    PWORD video = (PWORD)0xB8000;
    for (int i = 0; msg[i]; i++) {
        video[i] = 0x0c00 | (BYTE)msg[i];
    }
    for (;;) {
        __asm__ volatile ("cli; hlt");
    }
}

__attribute__((section(".text.start")))
void monitor_main(void) {

    if (lw_abi_attach((PVOID)LW_ABI_BASE) != 0) {
        no_abi_halt();
    }
    lw_puts("LWOS MONITOR v2 COPYLEFT 2026\n\r");

    idt_init();

    while (1);
}
