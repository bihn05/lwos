// physical memory manager

#ifndef _PMM_H_
#define _PMM_H_

#include <stdint.h>
#include <kernel.h>

#define ARDS_TYPE_USABLE 1
#define ARDS_TYPE_RESERVED 2

#define PAGE_DIR_BASE 0x100000
#define PAGE_TABLE_BASE 0x101000
#define PAGE_SIZE 0x1000

// ards structure
typedef struct {
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
} ards_t;

// pmm global manager structure
typedef struct {
    uint32_t map_start_addr; // bitmap mm addr
    uint32_t total_pages; // total mm pages
    uint32_t free_pages; // current usable pages
} pmm_manager_t;

pmm_manager_t pmm_manager;
uint8_t* pmm_bitmap; // bitmap ptr

void pmm_init();
void pmm_reserve_region(uint32_t start_addr, uint32_t size);
void pmm_reserve_page(uint32_t addr);
uint32_t pmm_alloc_page();
void pmm_free_page(uint32_t phy_addr);

// initialize physical memory manager
void pmm_init() {

    // get ards info
    uint32_t ards_count = *(uint32_t*)0x7e00;
    ards_t* ards_buffer = (ards_t*)0x7e10;

    uint64_t max_memory = 0;

    // calculate max mm
    for (int i = 0; i < ards_count; i++) {
        uint64_t region_end = ards_buffer[i].base_addr + ards_buffer[i].length;
        if (ards_buffer[i].type == ARDS_TYPE_USABLE && region_end > max_memory) {
            max_memory = region_end;
        }
    }

    pmm_manager.total_pages = max_memory / PAGE_SIZE;
    pmm_manager.free_pages = 0;

    // set bitmap pos
    pmm_manager.map_start_addr = 0x200000;
    pmm_bitmap = (uint8_t*)pmm_manager.map_start_addr;

    // calculate bitmap size in bytes
    uint32_t bitmap_size = pmm_manager.total_pages / 8;

    // initialize bitmap
    for (uint32_t i = 0; i < bitmap_size; i++) {
        pmm_bitmap[i] = 0xff;
    }

    // release usable as ards
    for (int i = 0;i < ards_count; i++) {
        if (ards_buffer[i].type == ARDS_TYPE_USABLE) {
            uint64_t base = ards_buffer[i].base_addr;
            uint64_t len = ards_buffer[i].length;

            // reset
            uint32_t start_page = base / PAGE_SIZE;
            uint32_t end_page = (base + len) / PAGE_SIZE;

            for (uint32_t page = start_page; page < end_page && page < pmm_manager.total_pages; page++) {
                pmm_bitmap[page / 8] &= ~(1 << (page % 8));
                pmm_manager.free_pages++;
            }
        }
    }

    printk("Max memory detected: %dMB\n", max_memory/0x400);

    // keep kernel & special
    // bios data
    pmm_reserve_region(0x0, 0x1000);
    // kernel
    pmm_reserve_region(0x10000, 0x90000);
    // display
    pmm_reserve_region(0xa0000, 0x50000);
    // page table
    pmm_reserve_page(0x100000);
    // bitmap
    pmm_reserve_region(0x200000, bitmap_size);

    printk("PMM Init: Bitmap @0x200000, Size=%d\n", bitmap_size);
}
void pmm_reserve_region(uint32_t start_addr, uint32_t size) {
    uint32_t start_page = start_addr / PAGE_SIZE;
    uint32_t end_page = (start_addr + size) / PAGE_SIZE;
    if ((start_addr + size) % PAGE_SIZE != 0)end_page++;

    for (uint32_t i = start_page; i < end_page; i++) {
        if (i < pmm_manager.total_pages) {
            if ((pmm_bitmap[i / 8] & (1 << (i % 8))) == 0) {
                pmm_bitmap[i / 8] |= (1 << (i % 8));
                pmm_manager.free_pages--;
            }
        }
    }
}
void pmm_reserve_page(uint32_t addr) {
    pmm_reserve_region(addr, PAGE_SIZE);
}
uint32_t pmm_alloc_page() {

    // find an unset bit
    for (uint32_t i = 0; i < pmm_manager.total_pages; i++) {
        if (!(pmm_bitmap[i / 8] & (1 << (i % 8)))) {
            pmm_bitmap[i / 8] |= (1 << (i % 8));
            pmm_manager.free_pages--;
            return i * PAGE_SIZE;
        }
    }
    return 0;
}
void pmm_free_page(uint32_t phy_addr) {
    uint32_t page_index = phy_addr / PAGE_SIZE;

    if (pmm_bitmap[page_index / 8] & (1 << (page_index % 8))) {
        pmm_bitmap[page_index / 8] &= ~(1 << (page_index % 8));
        pmm_manager.free_pages++;
    }
}

#endif