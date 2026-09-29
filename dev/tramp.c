#include "tramp.h"
#include "binfo.h"
#include "text.h"

_Static_assert(__builtin_offsetof(TRAMP_REGS, ax)    == 0x00, "ax");
_Static_assert(__builtin_offsetof(TRAMP_REGS, bx)    == 0x02, "bx");
_Static_assert(__builtin_offsetof(TRAMP_REGS, cx)    == 0x04, "cx");
_Static_assert(__builtin_offsetof(TRAMP_REGS, dx)    == 0x06, "dx");
_Static_assert(__builtin_offsetof(TRAMP_REGS, es)    == 0x08, "es");
_Static_assert(__builtin_offsetof(TRAMP_REGS, di)    == 0x0A, "di");
_Static_assert(__builtin_offsetof(TRAMP_REGS, bp)    == 0x0C, "bp");
_Static_assert(__builtin_offsetof(TRAMP_REGS, flags) == 0x0E, "flags");
_Static_assert(sizeof(TRAMP_REGS) == 0x10, "register block size");
_Static_assert(TRAMP_BLK_BASE + TRAMP_REGS_OFF == 0xA018u, "TRAMP_REG_PTR address");

static int gfx_is_live;

// stage2 is linked separately, reach it through the vector it left behind
static int tramp_int10(void) {
    DWORD v = TRAMP_VEC;
    if (!v) {
        return -1;
    }
    ((void (*)(void))v)();
    return 0;
}

WORD tramp_call(WORD ax, WORD bx, WORD cx, WORD dx) {
    TRAMP_REG_PTR->ax = ax;
    TRAMP_REG_PTR->bx = bx;
    TRAMP_REG_PTR->cx = cx;
    TRAMP_REG_PTR->dx = dx;
    TRAMP_REG_PTR->es = 0;
    TRAMP_REG_PTR->di = 0;
    TRAMP_REG_PTR->bp = 0;

    if (tramp_int10() != 0) {
        return 0xffff;
    }
    return TRAMP_REG_PTR->ax;
}

int gfx_enter(void) {
    DWORD f;
    WORD mode;
    if (!binfo_valid()) {
        return -1;
    }
    f = BINFO_PTR->flags;
    if ((f&(BINFO_F_VBE|BINFO_F_LFB))!=(BINFO_F_VBE|BINFO_F_LFB)) {
        return -1;
    }
    mode = BINFO_PTR->vbe_mode;
    if (!mode) {
        return -1;
    }
    if (tramp_call(0x4f02, (WORD)(mode|0x4000),0,0)!=0x004f) {
        return -2;
    }
    gfx_is_live = 1;
    return 0;
}

void gfx_exit(void) {
    tramp_call(0x0003,0,0,0);
    gfx_is_live=0;
    screen_clear();
}

int gfx_live(void) {
    return gfx_is_live;
}