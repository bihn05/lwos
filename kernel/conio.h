#ifndef _CONIO_H
#define _CONIO_H

#include <driver/kbc.h>

char getch() {
	while (kbd.count == 0) {
		;
		//asm volatile("hlt");
	}
	return get_char_buffer();
}
char getch_nb() {
	if (kbd.count == 0) {
		return 0;
	}
	return get_char_buffer();
}
uint8_t getsc() {
	while (kbd.sc_count == 0) {
		asm volatile("hlt");
	}
	return get_scancode_buffer();
}
void flush_keyboard() {
	kbd.head = kbd.tail = kbd.count = 0;
	kbd.sc_head = kbd.sc_tail = kbd.sc_count = 0;
}

#endif