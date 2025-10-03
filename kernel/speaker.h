#ifndef _PC_SPK_H
#define _PC_SPK_H

#include <interrupt.h>
#include <pic.h>
#include <driver/io.h>

#define SPK_PORT 0x61
#define PIT_CH2 0x42
#define PIT_CTRL 0x43
#define PIT_BASE_FREQ 1193182 // 计时器输入频率

void enable_spk() {
	uint8_t status = inb(SPK_PORT);
	status |= 0x03;
	outb(status, SPK_PORT);
}
void disable_spk() {
	uint8_t status = inb(SPK_PORT);
	status &= 0xfc;
	outb(status, SPK_PORT);
}
void set_spk_freq(uint16_t freq) {
	uint16_t reload_value = PIT_BASE_FREQ / freq;

	outb(0xb6, PIT_CTRL);
	outb(reload_value & 0xFF, PIT_CH2);
	outb((reload_value >> 8) & 0xFF, PIT_CH2);
}

#endif
