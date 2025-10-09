#ifndef _CHAR_PROCESS_H
#define _CHAR_PROCESS_H

#include <stdint.h>
#include <driver/video.h>

void put_func_char(char c) {
	draw_font(c);
	CursorX++;
	if (CursorX >= 80) {
		CursorX = 0;
		CursorY++;
		if (CursorY >= 30) {
			screen_scroll();
			CursorY = 29;
		}
	}
}

#endif
