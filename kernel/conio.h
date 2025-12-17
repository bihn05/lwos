#ifndef _CONIO_H
#define _CONIO_H

#include <stdint.h>
#include <driver/kbc.h>

char getch(void) {
	while (kb_buffer_is_emtpy(&kbd_buffer)) {
		__asm volatile("nop");
	}
	return buffer_get(&kbd_buffer);
}
int try_getch(void) {
	if (kb_buffer_is_emtpy(&kbd_buffer)) {
		return -1;
	}
	return (int)buffer_get(&kbd_buffer);
}
bool kbhit(void) {
	return !kb_buffer_is_emtpy(&kbd_buffer);
}

#endif