#ifndef _VMM_H
#define _VMM_H

#include <stdint.h>
#include <string.h>
#include <mm/pmm.h>
#include <mm/pt.h>

// 64位地址切分宏：每次取 9 位 (0x1FF)
#define PML4_INDEX(addr) (((uint64_t)(addr) >> 39) & 0x1FF)
#define PDPT_INDEX(addr) (((uint64_t)(addr) >> 30) & 0x1FF)
#define PD_INDEX(addr)   (((uint64_t)(addr) >> 21) & 0x1FF)
#define PT_INDEX(addr)   (((uint64_t)(addr) >> 12) & 0x1FF)

uint64_t get_cr3();
void flush_tlb(uint64_t vaddr);
int vmm_map_page(uint64_t pml4_pa, uint64_t va, uint64_t pa, uint64_t flags);
int vmm_alloc_map_region(uint64_t pml4_pa, uint64_t vaddr, uint64_t size, uint32_t flags);
int vmm_unmap_page(uint64_t pml4_pa, uint64_t va);

#endif