#ifndef _VIDEO_H
#define _VIDEO_H

#include <stdint.h>
#include <driver/port.h>

#define VGA_MISC_PORT		0x3C2
#define VGA_SEQ_INDEX_PORT	0x3C4
#define VGA_SEQ_DATA_PORT	0x3C5
#define VGA_CRTC_INDEX_PORT	0x3D4
#define VGA_CRTC_DATA_PORT	0x3D5
#define VGA_GC_INDEX_PORT	0x3CE
#define VGA_GC_DATA_PORT	0x3CF
#define VGA_AC_INDEX_PORT	0x3C0
#define VGA_AC_DATA_PORT	0x3C0
#define VGA_INSTAT_READ		0x3DA

typedef struct {
    uint32_t cursor_x;
    uint32_t cursor_y;
    uint8_t* frame_buf;
} video_t;

extern uint8_t g_640x480x2[];
extern unsigned char g_8x16_font[2048];

void writereg_video(uint8_t* regs);

void video_clear(video_t* v);
void video_screen_scroll(video_t* v);
void draw_font(video_t* v, uint32_t ch);
void draw_font_c(video_t* v, uint32_t ch, int color);
void putchar(video_t* v, char ch);
void putchar_c(video_t* v, char ch, int color);

#endif