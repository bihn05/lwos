#ifndef _PT_H
#define _PT_H

#include <mm/proc.h>
#include <mm/pmm.h>

#include <stdint.h>
#include <string.h>

static inline uint16_t pml4_index(uint64_t va) { return (va >> 39) & 0x1FF; }
static inline uint16_t pdpt_index(uint64_t va) { return (va >> 30) & 0x1FF; }
static inline uint16_t pd_index  (uint64_t va) { return (va >> 21) & 0x1FF; }
static inline uint16_t pt_index  (uint64_t va) { return (va >> 12) & 0x1FF; }
static inline void* pa_to_ptr(uint64_t pa) {
    return (void*)(uintptr_t)pa;
}
static inline uint64_t entry_addr(uint64_t entry) {
    return entry & 0xFFFFFFFFFFFFF000ULL;
}
static inline uint64_t pte_addr(uint64_t entry);
static uint64_t* pt_get_table_from_entry(uint64_t entry);
static uint64_t* pt_next_table(uint64_t* table, uint16_t index, int create, uint64_t upper_flags);
uint64_t* pt_get_pte(uint64_t pml4_pa, uint64_t va, int create, uint64_t upper_flags);
int pt_query(uint64_t pml4_pa, uint64_t va, uint64_t* out_pte);
void pt_debug_walk(uint64_t pml4_pa, uint64_t va);
static int split_2m_page(uint64_t* pd, uint16_t index);

#endif