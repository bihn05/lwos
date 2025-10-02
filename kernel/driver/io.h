#ifndef _PORT_H
#define _PORT_H

#include <stdint.h>

static inline void outb(uint8_t value, uint16_t port) {
	asm volatile ("outb %b0, %w1" : : "a"(value), "d"(port));
}
static inline void outw(uint16_t value, uint16_t port) {
	asm volatile ("outw %w0, %w1" : : "a"(value), "d"(port));
}
static inline void outl(uint32_t value, uint16_t port) {
	asm volatile ("outl %0, %w1" : : "a"(value), "d"(port));
}
static inline uint8_t inb(uint16_t port) {
	uint8_t data;
	asm volatile ("inb %w1, %b0" : "=a" (data) : "d" (port));
	return data;
}
static inline uint16_t inw(uint16_t port) {
	uint16_t data;
	asm volatile ("inw %w1, %w0" : "=a" (data) : "d" (port));
	return data;
}
static inline uint32_t inl(uint16_t port) {
	uint32_t data;
	asm volatile ("inl %w1, %0" : "=a" (data) : "d" (port));
	return data;
}

#endif
