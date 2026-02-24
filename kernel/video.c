#include <print.h>
#include <string.h>
uint8_t g_640x480x2[] =
{
/* MISC */
	0xE3,
/* SEQ */
	0x03, 0x01, 0x0F, 0x00, 0x06,
/* CRTC */
	0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0x0B, 0x3E,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xEA, 0x0C, 0xDF, 0x28, 0x00, 0xE7, 0x04, 0xE3,
	0xFF,
/* GC */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x0F,
	0xFF,
/* AC */
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
	0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
	0x01, 0x00, 0x0F, 0x00, 0x00
};
unsigned char g_8x16_font[2048] =
{
    #include "font.inc"
};
void writereg_video(uint8_t* regs) {
	uint16_t i;
	outb(*regs, VGA_MISC_PORT);
	regs++;

	for (i=0; i<5; i++) {
		outb(i, VGA_SEQ_INDEX_PORT);
		outb(*regs, VGA_SEQ_DATA_PORT);
		regs++;
	}
	outb(0x03, VGA_CRTC_INDEX_PORT);
        outb(inb(VGA_CRTC_DATA_PORT) | 0x80, VGA_CRTC_DATA_PORT);
        outb(0x11, VGA_CRTC_INDEX_PORT);
        outb(inb(VGA_CRTC_DATA_PORT) & 0x7f, VGA_CRTC_DATA_PORT);
	for (i = 0; i < 25; i++) {
		outb(i, VGA_CRTC_INDEX_PORT);
		outb(*regs, VGA_CRTC_DATA_PORT);
		regs++;
	}
	for(i = 0; i < 9; i++) {
		outb(i, VGA_GC_INDEX_PORT);
		outb(*regs, VGA_GC_DATA_PORT);
		regs++;
	}
	for(i = 0; i < 21; i++) {
		(void)inb(VGA_INSTAT_READ);
		outb(i, VGA_AC_INDEX_PORT);
		outb(*regs, VGA_AC_DATA_PORT);
		regs++;
	}
	(void)inb(VGA_INSTAT_READ);
	outb(0x20, VGA_AC_INDEX_PORT);
}
void video_clear(video_t* v) {
    memset(v->frame_buf, 0, 38400);
    v->cursor_x = 0;
    v->cursor_y = 0;
}
void video_screen_scroll(video_t* v) {
    int i = 0;
    for (; i < 37120; i++)
        v->frame_buf[i] = v->frame_buf[i + 1280];
    for (; i < 38400; i++)
        v->frame_buf[i] = 0;
}
void draw_font(video_t* v, uint32_t ch) {
    for (int i = 0; i < 16; i++)
        v->frame_buf[v->cursor_x + 80 * (v->cursor_y * 16 + i)] = g_8x16_font[ch * 16 + i];
}
void draw_font_c(video_t* v, uint32_t ch, int color) {
    if (color == 0)
        for (int i = 0; i < 16; i++)
            v->frame_buf[v->cursor_x + 80 * (v->cursor_y * 16 + i)] = g_8x16_font[ch * 16 + i];
    else
        for (int i = 0; i < 16; i++)
            v->frame_buf[v->cursor_x + 80 * (v->cursor_y * 16 + i)] = g_8x16_font[ch * 16 + i] ^ 0xff;
}
void putchar(video_t* v, char ch) {
    if (ch == '\b') {
        v->cursor_x--;
        if (v->cursor_x == -1)v->cursor_x = 0;
        return;
    }
    if ((ch == 0xd) || (ch == 0xa)) {
        v->cursor_y++;
        if (v->cursor_y >= 30) {
            video_screen_scroll(v);
            v->cursor_y = 29;
        }
        v->cursor_x = 0;
        return;
    }
    draw_font(v, ch);
    v->cursor_x++;
    if (v->cursor_x >= 80) {
        v->cursor_x = 0;
        v->cursor_y++;
        if (v->cursor_y >= 30) {
            video_screen_scroll(v);
            v->cursor_y = 29;
        }
    }
}
void putchar_c(video_t* v, char ch, int color) {
    if (ch == '\b') {
        v->cursor_x--;
        if (v->cursor_x == -1)v->cursor_x = 0;
        return;
    }
    if ((ch == 0xd) || (ch == 0xa)) {
        v->cursor_y++;
        if (v->cursor_y >= 30) {
            video_screen_scroll(v);
            v->cursor_y = 29;
        }
        v->cursor_x = 0;
        return;
    }
    draw_font_c(v, ch, color);
    v->cursor_x++;
    if (v->cursor_x >= 80) {
        v->cursor_x = 0;
        v->cursor_y++;
        if (v->cursor_y >= 30) {
            video_screen_scroll(v);
            v->cursor_y = 29;
        }
    }
}