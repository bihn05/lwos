#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include <driver/io.h>
#include <interrupt.h>

#define KEYBOARD_DAT 0x60
#define KEYBOARD_CMD 0x64

extern void set_interrupt_mask(uint8_t vector, uint8_t status);
extern void test(int a, int b);
extern void keyboard_hd(void);
void keyboard_handler(void) {
	send_eoi(1);
	uint16_t sc = inb(KEYBOARD_DAT);
	printk("Keyboard Input 0x%02x\n", sc);
}
void keyboard_init() {
	idt_set_gate(0x21, (uint32_t)keyboard_hd, 0x08, 0x8e);
	idt_flush();
	IRQ_clear_mask(1);
}


#endif
