#ifndef _KEYBOARD_H_
#define _KEYBOARD_H_

#include <port.h>
#include <stdint.h>

void InitKb() {
	outb(0xad, 0x64); // disable keyboard
	outb(0xa7, 0x64);

	while (inb(0x64)&0x01) {
		inb(0x60);
	}

	outb(0xae, 0x64);
}

uint8_t ReadScancode() {
	uint8_t status = inb(0x64);
	while (!(status & 0x01)) {
		status = inb(0x64);
	}
	return inb(0x60);
}
void ProcScancode(uint8_t scancode) {
	if ((scancode & 0x80) == 0x80) {
		outstr("\nKey released : 0x");
		iouthex8(scancode&0x7f);
	} else {
		outstr("\nKey pressed : 0x");
		iouthex8(scancode);
	}
}

#endif
