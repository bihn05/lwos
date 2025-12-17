#ifndef _MEM_H
#define _MEM_H

#include <stdint.h>
#include <string.h>
#include <kernel.h>
#include <service/char_proc.h>

void dump_chunk(void* addr, uint32_t len) {
	printk("Your position 0x%08x\n", (uint32_t)addr);
	uint8_t* p = (uint8_t*)((uint32_t)addr & 0xfffffff0);
	for (uint32_t t = 0; t < len; t++) {
		for (uint32_t i = 0; i < 16; i++) {
			printk("%08x:", (uint32_t)((uint32_t)addr & 0xfffffff0) + i * 16);
			for (uint32_t j = 0; j < 16; j++) {
				printk("%02x ", p[i * 16 + j + t * 256]);
			}
			printk("\b|");
			for (uint32_t j = 0; j < 16; j++) {
				put_func_char(p[i * 16 + j + t * 256]);
			}
			printk("\n");
		}
	}
}


#endif