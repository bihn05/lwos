#include "ctx.h"
#include "abi.h"
#include "io.h"
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
    "#DE divide error",
    "#DB debug",
    "NMI",
    "#BP breakpoint",

    "#OF overflow",
    "#BR bound range",
    "#UD invalid opcode"
    "#NM device n/a",

    "#DF double fault",
    "coproc seg overflow",
    "#TS invalid TSS",
    "#NP segment n/p",

    "#SS stack fault",
    "#GP general prot",
    "#PF page fault",
    "reserved",

    "#MF fpu error",
    "#AC align check",
    "#MC machine check",
    "#XM SIMD ERROR",

    "#VE virt exception",
    "#CP ctrl protection"
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
    outb(0x21, 0xFF);
	outb(0xA1, 0xFF);
}

void dbg_panic(struct ctx *c)
{
	lw_puts("\r\n\r\n*** UNRECOVERABLE FAULT ***\n");
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
