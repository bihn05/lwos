#ifndef _MEM_H
#define _MEM_H

#include <stdint.h>
#include <string.h>
#include <kernel.h>

#define ARDS_TYPE_USABLE 1
#define ARDS_TYPE_RESERVED 2

#define PAGE_DIR_BASE 0x100000
#define PAGE_TABLE_BASE 0x101000
#define PAGE_SIZE 4096

void dump_chunk(void* addr, uint32_t len) {
	printk("Your position 0x%08x\n", (uint32_t)addr);
	uint8_t* p = (uint8_t*)((uint32_t)addr & 0xfffffff0);
	for (uint32_t t = 0; t < len; t++) {
		for (uint32_t i = 0; i < 16; i++) {
			printk("%08x:", (uint32_t)((uint32_t)addr & 0xfffffff0) + i * 16);
			for (uint32_t j = 0; j < 16; j++) {
				printk("%02x ", p[i * 16 + j + t * 256]);
			}
			printk("\n");
		}
	}
}

#pragma pack(push, 1)
typedef struct {
	uint64_t base;
	uint64_t length;
	uint32_t type;
} ards_t;
typedef struct {
	uint32_t present : 1;  // Page present in memory
	uint32_t rw : 1;       // Read-only if clear, readwrite if set
	uint32_t user : 1;     // Supervisor level only if clear
	uint32_t pwt : 1;      // Page-level write-through
	uint32_t pcd : 1;      // Page-level cache disable
	uint32_t accessed : 1; // Has the page been accessed since last refresh?
	uint32_t dirty : 1;    // Has the page been written to since last refresh?
	uint32_t pat : 1;      // Page Attribute Table
	uint32_t global : 1;   // Global page (ignored if CR4.PGE = 0)
	uint32_t avail : 3;    // Available for software use
	uint32_t frame : 20;   // Frame address (shifted right 12 bits)
} PE;
#pragma pack(pop)

uint32_t get_cr2(void);
uint32_t get_cr3(void);
void init_paging(void);
void enable_paging(void);
void setup_mem(void);

uint64_t calc_mem(void) {
	uint32_t* ards_count_ptr = (uint32_t*)0x7e00;
	uint32_t ards_count = *ards_count_ptr;
	ards_t* ards_ptr = (ards_t*)0x7e10;

	uint64_t total_mem = 0;

	for (uint32_t i = 0; i < ards_count; i++) {
		ards_t* ards = &ards_ptr[i];

		uint64_t length = (uint64_t)ards->length;

		if (ards->type == ARDS_TYPE_USABLE) {
			total_mem += length;
		}
		
		printk("ARDS[%d]: base=0x%016x, ",
			i, (uint64_t)ards->base);
		printk("lgth=0x%016x, ",
			(uint64_t)length);
		printk("type=%d\n",
			ards->type);
	}
	printk("USABLE MEMORY %dKB\n", total_mem >> 10);
	return total_mem;
}

uint32_t get_cr2() {
	asm volatile("movl %cr2, %eax");
}
uint32_t get_cr3() {
	asm volatile("movl %cr3, %eax");
}

#endif