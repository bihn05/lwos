#ifndef _INT_H
#define _INT_H

#include <pristdio.h>
#include <port.h>
#include <stdint.h>
#include <global.h>
#include <debug.h>

#define IDT_SIZE 256

struct gate_t {
	uint16_t offset0;
	uint16_t selector;
	uint8_t reserved;
	uint8_t type : 4;
	uint8_t segment : 1;
	uint8_t dpl : 2;
	uint8_t present : 1;
	uint16_t offset1;
};

struct gate_t idt[IDT_SIZE];
struct pointer_t idt_ptr;
extern void interrupt_handler();

void interrupt_init() {
	for (size_t i = 0; i < IDT_SIZE; i++) {
		gate_t* gate = &idt[i];
		gate->offset0 = (uint32_t)interrupt_handler & 0xffff;
		gate->offset1 = (uint32_t)(interrupt_handler>>16) & 0xffff;
		gate->selector = 0x08;
		gate->reserved = 0;
		gate->type = 0xe;
		gate->segment = 0;
		gate->dpl = 0;
		gate->present = 1;
	}
	idt_ptr.base = (uint32_t)idt;
	idt_ptr.limit = sizeof(idt) - 1;
	BMB;
	asm volatile("lidt idt_ptr\n");
}

#endif
