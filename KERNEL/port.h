#ifndef PORT_H
#define PORT_H

#include <stdint.h>

void outb(uint8_t value, uint16_t port) {
	asm volatile ("outb %b0, %w1" : : "a"(value), "d"(port));
}
void outw(uint16_t value, uint16_t port) {
	asm volatile ("outw %w0, %w1" : : "a"(value), "d"(port));
}
void outl(uint32_t value, uint16_t port) {
	asm volatile ("outl %0, %w1" : : "a"(value), "d"(port));
}
uint8_t inb(uint16_t port) {
	uint8_t data;
	asm volatile ("inb %w1, %b0" : "=a" (data) : "d" (port));
	return data;
}
uint16_t inw(uint16_t port) {
	uint16_t data;
	asm volatile ("inw %w1, %w0" : "=a" (data) : "d" (port));
	return data;
}
uint32_t inl(uint16_t port) {
	uint32_t data;
	asm volatile ("inl %w1, %0" : "=a" (data) : "d" (port));
	return data;
}

#endif
