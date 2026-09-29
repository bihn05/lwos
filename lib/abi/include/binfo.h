#ifndef _LW_BINFO_H
#define _LW_BINFO_H

#include "stdint.h"
#include "io.h"

#define BINFO_BASE      0x9000u
#define BINFO_MAGIC     0x4F464E42u     /* 'BNFO' little-endian */

#define BINFO_O_MAGIC       0x00    /* u32  BINFO_MAGIC once the harvest ran */
#define BINFO_O_SIZE        0x04    /* u32  bytes filled */
#define BINFO_O_FLAGS       0x08    /* u32  BINFO_F_* */
#define BINFO_O_VBE_MODE    0x0C    /* u16  mode number actually set, 0 if none */
#define BINFO_O_FB_PHYS     0x10    /* u32  linear framebuffer physical address */
#define BINFO_O_FB_PITCH    0x14    /* u32  bytes per scanline */
#define BINFO_O_FB_W        0x18    /* u16  pixels */
#define BINFO_O_FB_H        0x1A    /* u16  pixels */
#define BINFO_O_FB_BPP      0x1C    /* u8   bits per pixel */
#define BINFO_O_FB_RED      0x1D    /* u8   red mask size, position in next */
#define BINFO_O_FB_REDPOS   0x1E    /* u8 */
#define BINFO_O_FB_GRN      0x1F    /* u8 */
#define BINFO_O_FB_GRNPOS   0x20    /* u8 */
#define BINFO_O_FB_BLU      0x21    /* u8 */
#define BINFO_O_FB_BLUPOS   0x22    /* u8 */
#define BINFO_O_GFX_BDF     0x24    /* u32  bus<<16|dev<<8|fn of the VGA class fn */
#define BINFO_O_GFX_IDS     0x28    /* u32  device<<16|vendor, e.g. a0118086 */
#define BINFO_O_GFX_IRQ     0x2C    /* u8   irq line byte from config 0x3C */
#define BINFO_O_GFX_BARS    0x30    /* u32[6] raw BAR values as the BIOS left them */
#define BINFO_O_GFX_SIZES   0x48    /* u32[6] sized lengths, 0 if unimplemented */
#define BINFO_O_EDID        0x60    /* 128 bytes, valid only if BINFO_F_EDID */
#define BINFO_O_VBE_VER     0xE0    /* u16  BCD, 0x0300 = VBE 3.0 */
#define BINFO_O_VBE_MEM     0xE2    /* u16  video memory in 64KB units */
#define BINFO_O_END         0xE4

#define BINFO_F_VBE     0x01        /* VBE present, mode fields filled */
#define BINFO_F_LFB     0x02        /* that mode has a linear framebuffer */
#define BINFO_F_EDID    0x04        /* int 10h AX=4F15h returned a block */
#define BINFO_F_GFX     0x08        /* a class-03 function was found */
#define BINFO_F_QUIET   0x10        /* GPU interrupts were masked before handoff */
#define BINFO_F_MODESET 0x20        /* 4F02 ran: the mode in vbe_mode is live */

#ifndef __ASSEMBLER__
/* Mirror of the offsets above. Kept in sync by the static asserts in binfo.c. */
typedef struct _BINFO{
    DWORD magic, size, flags;
    WORD  vbe_mode;
    WORD  _pad0;
    DWORD fb_phys, fb_pitch;
    WORD  fb_w, fb_h;
    BYTE  fb_bpp;
    BYTE  fb_red, fb_redpos, fb_grn, fb_grnpos, fb_blu, fb_blupos;
    BYTE  _pad1;
    DWORD gfx_bdf, gfx_ids;
    BYTE  gfx_irq;
    BYTE  _pad2[3];
    DWORD gfx_bars[6], gfx_sizes[6];
    BYTE  edid[128];
    WORD  vbe_ver, vbe_mem64k;
} BINFO, *PBINFO;
typedef const BINFO *PCBINFO;

#define BINFO_PTR ((PCBINFO)BINFO_BASE)
int  binfo_valid(void);
void binfo_dump(void);
DWORD bi_fb(void);
DWORD bi_fb_w(void);
DWORD bi_fb_h(void);
DWORD bi_fb_bpp(void);
#endif

#endif