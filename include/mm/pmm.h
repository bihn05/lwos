// physical memory manager (64-bit Edition)

#ifndef _PMM_H_
#define _PMM_H_

#include <stdint.h>
#include <printk.h>

#define ARDS_TYPE_USABLE 1
#define ARDS_TYPE_RESERVED 2

#define PAGE_SIZE 0x1000 // 4KB

// ards 结构保持不变，已经是 64 位兼容的
typedef struct {
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
} __attribute__((packed)) ards_t;

// PMM 全局管理结构 (全面升级为 64 位)
typedef struct {
    uint64_t map_start_addr; // 位图在内存中的起始物理地址
    uint64_t total_pages;    // 总物理页数
    uint64_t free_pages;     // 当前可用物理页数
} pmm_manager_t;

extern pmm_manager_t pmm_manager;
extern uint8_t* pmm_bitmap; // 位图指针

// 函数声明：地址和参数全面改为 uint64_t
void pmm_init();
void pmm_reserve_region(uint64_t start_addr, uint64_t size);
void pmm_reserve_page(uint64_t addr);
uint64_t pmm_alloc_page();
void pmm_free_page(uint64_t phy_addr);
void print_pmm_info();

#endif