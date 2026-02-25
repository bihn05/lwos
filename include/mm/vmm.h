#ifndef _VMM_H
#define _VMM_H

#include <stdint.h>
#include <string.h>
#include <printk.h>
#include <mm/pmm.h>

#define PAGE_PRESENT 0x1
#define PAGE_RW      0x2
#define PAGE_USER    0x4

// 64位地址切分宏：每次取 9 位 (0x1FF)
#define PML4_INDEX(addr) (((uint64_t)(addr) >> 39) & 0x1FF)
#define PDPT_INDEX(addr) (((uint64_t)(addr) >> 30) & 0x1FF)
#define PD_INDEX(addr)   (((uint64_t)(addr) >> 21) & 0x1FF)
#define PT_INDEX(addr)   (((uint64_t)(addr) >> 12) & 0x1FF)

uint64_t get_cr3();
void flust_tlb(uint64_t vaddr);
void map_page(uint64_t vaddr, uint64_t paddr, uint32_t flags);
void vmm_alloc_map_region(uint64_t vaddr, uint64_t size, uint32_t flags);

#endif