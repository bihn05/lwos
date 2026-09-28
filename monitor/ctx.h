#ifndef _LW_CTX_H
#define _LW_CTX_H
#include "io.h"
#include "stdint.h"

struct ctx {
    DWORD eax, ecx, edx, ebx;
    DWORD esp, ebp, esi, edi;
    DWORD eip, eflags;
    DWORD cs, ds, es, fs, gs, ss;
    DWORD vector, errcode;
};

#define EFLAG_TF 0x00000100U
#define EFLAG_IF 0x00000200U

void dbg_enter(struct ctx *c);

extern struct ctx* cur_ctx;
extern DWORD in_debuggee;
extern DWORD isr_table[];

void idt_init(void);
const char* vec_name(DWORD v);

#endif