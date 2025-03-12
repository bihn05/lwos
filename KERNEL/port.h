#ifndef PORT_H
#define PORT_H

#include <stdint.h>

void outb(uint8_t value, uint16_t port) {
	asm volatile ("outb %b0, %w1" : : "a"(value), "d"(port));
	asm volatile ("nop");
	asm volatile ("nop");
	asm volatile ("nop");
}
uint8_t inb(uint16_t port) {
	uint8_t data;
	asm volatile ("inb %w1, %b0" : "=a" (data) : "d" (port));
	asm volatile ("nop");
	asm volatile ("nop");
	asm volatile ("nop");
	return data;
}

#endif
