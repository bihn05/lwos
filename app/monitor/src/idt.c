#include "ctx.h"
#include "abi.h"
#include "stdint.h"

#define NVEC        48
#define CODE_SEL    0x08

struct gate {
    WORD    off_lo;
    WORD    sel;
    BYTE    zero;
    BYTE    flags;
    WORD    off_hi;
} __attribute__((packed));

static struct gate idt[NVEC];

static struct {
    WORD  limit;
    DWORD base;
} __attribute__((packed)) idtr;

static const char* const names[] = {
    "#DE divide error\0",
    "#DB debug\0",
    "NMI\0",
    "#BP breakpoint\0",

    "#OF overflow\0",
    "#BR bound range\0",
    "#UD invalid opcode\0"
    "#NM device n/a\0",

    "#DF double fault\0",
    "coproc seg overflow\0",
    "#TS invalid TSS\0",
    "#NP segment n/p\0",

    "#SS stack fault\0",
    "#GP general prot\0",
    "#PF page fault\0",
    "reserved\0",

    "#MF fpu error\0",
    "#AC align check\0",
    "#MC machine check\0",
    "#XM SIMD ERROR\0",

    "#VE virt exception\0",
    "#CP ctrl protection\0"
};

const char* vec_name(DWORD v) {
    if (v < sizeof(names) / sizeof(names[0]))
        return names[v];
    if (v >= 32)
        return "IRQ";
    return "reserved";
}

void idt_init(void) {
    for (int i = 0; i < NVEC; i++) {
        DWORD h = isr_table[i];
        idt[i].off_lo = (WORD)(h & 0xffff);
        idt[i].off_hi = (WORD)(h >> 16);
        idt[i].sel    = CODE_SEL;
        idt[i].zero   = 0;
        idt[i].flags  = 0x8e;
    }

    idtr.limit = sizeof(idt) - 1;
    idtr.base = (DWORD)(&idt[0]);
	__asm__ volatile ("lidt %0" :: "m"(idtr));

    // masked everything
    lw_outb(0x21, 0xff);
    lw_outb(0xa1, 0xff);
}

void dbg_panic(struct ctx *c)
{
	lw_puts("\r\n\r\n*** UNRECOVERABLE FAULT ***\n\r");
	lw_puts("VECTOR "); lw_put_dword(c->vector);
	lw_puts(" ");       lw_puts((char*)vec_name(c->vector));
	lw_puts("  ERRCODE "); lw_put_dword(c->errcode);
	lw_puts("\r\nEIP "); lw_put_dword(c->eip);
	lw_puts(" ESP ");  lw_put_dword(c->esp);
	lw_puts(" EFL ");  lw_put_dword(c->eflags);
	lw_puts("\r\n");
	if (c->vector == 14) {
		DWORD cr2;
		__asm__ volatile ("mov %%cr2, %0" : "=r"(cr2));
		lw_puts("CR2 "); lw_put_dword(cr2); lw_puts("\r\n");
	}
	lw_puts("RESET NOW\r\n");
}
