#include "binfo.h"
#include "text.h"

_Static_assert(__builtin_offsetof(BINFO, magic)    == BINFO_O_MAGIC,    "magic");
_Static_assert(__builtin_offsetof(BINFO, size)     == BINFO_O_SIZE,     "size");
_Static_assert(__builtin_offsetof(BINFO, flags)    == BINFO_O_FLAGS,    "flags");
_Static_assert(__builtin_offsetof(BINFO, vbe_mode) == BINFO_O_VBE_MODE, "vbe_mode");
_Static_assert(__builtin_offsetof(BINFO, fb_phys)  == BINFO_O_FB_PHYS,  "fb_phys");
_Static_assert(__builtin_offsetof(BINFO, fb_pitch) == BINFO_O_FB_PITCH, "fb_pitch");
_Static_assert(__builtin_offsetof(BINFO, fb_w)     == BINFO_O_FB_W,     "fb_w");
_Static_assert(__builtin_offsetof(BINFO, fb_h)     == BINFO_O_FB_H,     "fb_h");
_Static_assert(__builtin_offsetof(BINFO, fb_bpp)   == BINFO_O_FB_BPP,   "fb_bpp");
_Static_assert(__builtin_offsetof(BINFO, fb_blupos)== BINFO_O_FB_BLUPOS,"fb_blupos");
_Static_assert(__builtin_offsetof(BINFO, gfx_bdf)  == BINFO_O_GFX_BDF,  "gfx_bdf");
_Static_assert(__builtin_offsetof(BINFO, gfx_ids)  == BINFO_O_GFX_IDS,  "gfx_ids");
_Static_assert(__builtin_offsetof(BINFO, gfx_irq)  == BINFO_O_GFX_IRQ,  "gfx_irq");
_Static_assert(__builtin_offsetof(BINFO, gfx_bars) == BINFO_O_GFX_BARS, "gfx_bars");
_Static_assert(__builtin_offsetof(BINFO, gfx_sizes)== BINFO_O_GFX_SIZES,"gfx_sizes");
_Static_assert(__builtin_offsetof(BINFO, edid)     == BINFO_O_EDID,     "edid");
_Static_assert(__builtin_offsetof(BINFO, vbe_ver)  == BINFO_O_VBE_VER,  "vbe_ver");
_Static_assert(__builtin_offsetof(BINFO, vbe_mem64k)==BINFO_O_VBE_MEM,  "vbe_mem");
_Static_assert(sizeof(BINFO) == BINFO_O_END, "block size");

int binfo_valid(void) {
    return BINFO_PTR->magic == BINFO_MAGIC;
}

void binfo_dump(void) {
    if (!binfo_valid()) {
        puts("NO BOOT INFO AT ");
        put_dword(BINFO_BASE);
        puts(", MAGIC=");
        put_dword(BINFO_PTR->magic);
        puts(")\n\r");
        return;
    }

    DWORD f = BINFO_PTR -> flags;
    puts("BOOT INFO AT ");
    put_dword(BINFO_BASE);
    puts("  SIZE=");
    put_dword(BINFO_PTR -> size);
    puts("  flags");
    put_dword(f);
    puts("\n\r");

    if (f&BINFO_F_VBE) {
        puts("  VBE  ");
        put_word(BINFO_PTR->vbe_ver);
        puts("  VRAM ");
        put_word((DWORD)BINFO_PTR->vbe_mem64k*64);
        puts("KB\n\r  MODE ");
        put_word(BINFO_PTR->vbe_mode);
        puts((f&BINFO_F_MODESET)?" (LIVE) ":" (CAND) ");
        put_word(BINFO_PTR->fb_w);
        puts("x");
        put_word(BINFO_PTR->fb_h);
        puts(" ");
        put_word(BINFO_PTR->fb_bpp);
        puts("  PITCH ");
        put_dword(BINFO_PTR->fb_pitch);
        puts("\n\r");

        if (f&BINFO_F_LFB) {
            puts("  LFB ");
            put_dword(BINFO_PTR->fb_phys);
            puts("  RGB ");
            putc(0x30+BINFO_PTR->fb_red);
            puts(":");
            put_byte(BINFO_PTR->fb_redpos);
            puts(" ");
            putc(0x30+BINFO_PTR->fb_grn);
            puts(":");
            put_byte(BINFO_PTR->fb_grnpos);
            puts(" ");
            putc(0x30+BINFO_PTR->fb_blu);
            puts(":");
            put_byte(BINFO_PTR->fb_blupos);
            puts("\n\r");
        } else {
            puts("  NO LINEAR FB\n\r");
        }
    } else {
        puts("  NO VBE\n\r");
    }

    if (f&BINFO_F_GFX) {
        DWORD b = BINFO_PTR->gfx_bdf;
        puts("  GFX  ");
        put_byte((BYTE)(b>>16));
        puts(":");
        put_byte((BYTE)(b>>8));
        puts(":");
        put_byte((BYTE)(b));
        puts(" ");
        put_word((WORD)BINFO_PTR->gfx_ids);
        puts(":");
        put_word((WORD)(BINFO_PTR->gfx_ids>>16));
        puts("  IRQ  ");
        put_byte(BINFO_PTR->gfx_irq);
        puts((f&BINFO_F_QUIET)?" (QUIET)":" (LIVE) ");
        for (int i = 0; i < 6; i++) {
            if (!BINFO_PTR->gfx_bars[i]) {
                continue;
            }
            puts("    BAR");
            put_dword(i);
            puts(" ");
            put_dword(BINFO_PTR->gfx_bars[i]);
            puts("  LEN ");
            put_dword(BINFO_PTR->gfx_sizes[i]);
            puts("\n\r");
        }
    }

    if (f&BINFO_F_EDID) {
        puts("  EDID PRST, 128B AT ");
        put_dword(BINFO_BASE+BINFO_O_EDID);
        puts("\n\r");
    }
}

DWORD bi_fb(void) {
    DWORD t=BINFO_PTR->fb_phys;
    return t;
}
DWORD bi_fb_w(void) {
    DWORD t=BINFO_PTR->fb_w;
    return t;
}
DWORD bi_fb_h(void) {
    DWORD t=BINFO_PTR->fb_h;
    return t;
}
DWORD bi_fb_bpp(void) {
    DWORD t=BINFO_PTR->fb_bpp;
    return t;
}
DWORD bi_fb_pitch(void) {
    DWORD t=BINFO_PTR->fb_pitch;
    return t;
}