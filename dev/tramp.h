#ifndef _LW_TRAMP_H
#define _LW_TRAMP_H

#include "io.h"
#include "stdint.h"

#define TRAMP_BLK_BASE  0xa000
#define TRAMP_REGS_OFF  0x18
#define TRAMP_VEC_OFF   0x28    /* DWORD, stage2 writes &tramp_int10 here */

#ifndef __ASSEMBLER__
typedef struct _TRAMP_REGS {
    WORD ax, bx, cx ,dx, es, di, bp, flags;
} TRAMP_REGS, *PTRAMP_REGS;

#define TRAMP_REG_PTR ((volatile PTRAMP_REGS)(TRAMP_BLK_BASE+TRAMP_REGS_OFF))
#define TRAMP_VEC     (*(volatile DWORD *)(TRAMP_BLK_BASE+TRAMP_VEC_OFF))

WORD tramp_call(WORD ax, WORD bx, WORD cx, WORD dx);

int gfx_enter(void);
void gfx_exit(void);
int gfx_live(void);

#endif

#endif