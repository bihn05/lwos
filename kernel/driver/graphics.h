/* LWOS GRAPHICS LIBRARY
 *	ONLY DESIGN FOR MONOCHROME VGA 640*480
 * 
 */

#ifndef _GRAPHICS_H
#define _GRAPHICS_H

#include <driver/video.h>

typedef struct {
	int16_t x;
	int16_t y;
} point_t;

void putpixel(int16_t x, int16_t y, uint8_t color);

void putpixel(int16_t x, int16_t y, uint8_t color) {
	uint32_t offset = (x >> 3) + y * 80;
	if (offset >= 38400)return;
	if (color == 0) {
		video[offset] &= ~(1 << (7 - (x % 8)));
	}
	else {
		video[offset] |= 1 << (7 - (x % 8));
	}
}
void draw_line_w(int16_t x, int16_t y, int16_t length, uint8_t color) {
	for (int i = x; i < x + length; i++) {
		putpixel(i, y, 1);
	}
}
void draw_line_h(int16_t x, int16_t y, int16_t length, uint8_t color) {
	for (int i = y; i < y + length; i++) {
		putpixel(x, i, 1);
	}
}

#endif