#ifndef _MEMORY_H_
#define _MEMORY_H_

#include <type.h>
#include <port.h>
#include <vsprintf.h>
#include <pristdio.h>
#include <bitmap.h>
#include <stdlib.h>

typedef struct ards_entry_t {
	unsigned long long base;
	unsigned long long lgth;
	unsigned int type;
//	unsigned int resv;
} ards_entry_t;
#define ZONE_VALID 1    // ards 可用内存区域
#define ZONE_RESERVED 2 // ards 不可用区域
#define IDX(addr) ((uint32_t)addr >> 12)            // 获取 addr 的页索引
#define DIDX(addr) (((uint32_t)addr >> 22) & 0x3ff) // 获取 addr 的页目录索引
#define TIDX(addr) (((uint32_t)addr >> 12) & 0x3ff) // 获取 addr 的页表索引
#define PAGE(idx) ((uint32_t)idx << 12)             // 获取页索引 idx 对应的页开始的位置
#define ASSERT_PAGE(addr) assert((addr & 0xfff) == 0)
#define PDE_MASK 0xFFC00000
#pragma pack(1)
typedef struct PE {
    uint32_t present : 1;  // 页表是否存在于内存中
    uint32_t read_write : 1;  // 0 = 只读, 1 = 可读可写
    uint32_t user_supervisor : 1;  // 0 = 超级用户, 1 = 普通用户
    uint32_t write_through : 1;  // 0 = 写回, 1 = 写直达
    uint32_t cache_disable : 1;  // 0 = 启用缓存, 1 = 禁用缓存
    uint32_t accessed : 1;  // 是否被访问过
    uint32_t reserved : 1;  // 保留位
    uint32_t page_size : 1;  // 0 = 4KB页, 1 = 4MB页（仅适用于PDE）
    uint32_t global : 1;  // 0 = 非全局, 1 = 全局（仅适用于PTE）
    uint32_t available : 3;  // 操作系统可用位
    uint32_t page_base : 20; // 页表基地址（物理地址的高20位）
} PE;
PE* pg_base = (uint32_t*)0x100000;
uint32_t* ards_count = 0x7e00;
ards_entry_t * ards_buffer = 0x7e10;

uint32_t total_mem() {
	uint32_t t;
	for (uint32_t i = 0; i < *ards_count; i++) {

		printk("ARDS #%d -", i);
		printk(" BASE = ");
		iouthex64(ards_buffer[i].base);
		printk(" LGTH = ");
		iouthex64(ards_buffer[i].lgth);
		printk(" TYPE = ");
		iouthex32(ards_buffer[i].type);
		printk("\n");

		if (ards_buffer[i].type == 1) {
			t += ards_buffer[i].lgth;
		}
	}
	return t;
}

static uint32_t KERNEL_PAGE_TABLE[] = {
    0x102000,
    0x103000,
    0x104000,
    0x105000
};
#define KERNEL_MAP_BITS 0x6000

bitmap_t kernel_map;

// available memory base addr sd=1m
static uint32_t memory_base = 0;
// available memory size
static uint32_t memory_size = 0;
// total pages amount
static uint32_t total_pages = 0;
// free pages amount
static uint32_t free_pages = 0;

#define used_pages (total_pages - free_pages)
#define PAGE_SIZE 0x1000     // 一页的大小 4K
#define MEMORY_BASE 0x100000 // 1M，可用内存开始的位置
#define MEMORY_SIZE 0x1000000 // 内核占用的内存大小 16M
#define RAMDISK_MEM 0xC00000

void memory_init() {
    ards_entry_t* ptr = ards_buffer;
    for (size_t i = 0; i < *ards_count; i++, ptr++) {
        printk("Memroy base 0x%p size 0x%p type %d\n",
            (uint32_t)ptr->base, (uint32_t)ptr->lgth, (uint32_t)ptr->type);
        if (ptr->type == ZONE_VALID && ptr->lgth > memory_size) {
            memory_base = (uint32_t)ptr->base;
            memory_size = (uint32_t)ptr->lgth;
        }
    }
    printk("Memory base 0x%p\n", (uint32_t)memory_base);
    printk("Memory size 0x%p\n", (uint32_t)memory_size);
    assert(memory_base == MEMORY_BASE); // 内存开始的位置为 1M
    assert((memory_size & 0xfff) == 0); // 要求按页对齐

    total_pages = IDX(memory_size) + IDX(MEMORY_BASE);
    free_pages = IDX(memory_size);

    if (memory_size < MEMORY_SIZE) {
        printk("!!! panic !!!\n-->  \n");
        printk("System memory is %dM too small, at least %dM needed\n",
            memory_size / MEMORY_BASE, MEMORY_SIZE / MEMORY_BASE);
        panic();
    }
}

static uint32_t start_page = 0;   // 可分配物理内存起始位置
static uint8_t* memory_map;       // 物理内存数组
static uint32_t memory_map_pages; // 物理内存数组占用的页数

void memory_map_init() {
    memory_map = (uint8_t*)memory_base;
    memory_map_pages = div_round_up(total_pages, PAGE_SIZE);
    printk("Memory map page count %d\n", memory_map_pages);
    free_pages -= memory_map_pages;
    memset((void*)memory_map, 0, memory_map_pages * PAGE_SIZE);
    start_page = IDX(MEMORY_BASE) + memory_map_pages;
    for (size_t i = 0; i < start_page; i++) {
        memory_map[i] = 1;
    }
    printk("Total pages &d free pages %d\n", total_pages, free_pages);
    uint32_t length = (IDX(RAMDISK_MEM) - IDX(MEMORY_BASE)) / 8;
    bitmap_init(&kernel_map,
        (uint8_t*)KERNEL_MAP_BITS, length, IDX(MEMORY_BASE));
    bitmap_scan(&kernel_map, memory_map_pages);
}
// 分配一页物理内存
static uint32_t get_page() {
    for (size_t i = start_page; i < total_pages; i++) {
        if (!memory_map[i]) {
            memory_map[i] = 1;
            assert(free_pages > 0);
            free_pages--;
            uint32_t page = PAGE(i);
            printk("GET page 0x%p\n", page);
            return page;
        }
    }
    printk("Out of memory Blyat!!!");
    panic();
}
// 释放一页内存
static void put_page(uint32_t addr) {
    ASSERT_PAGE(addr);
    uint32_t idx = IDX(addr);
    assert(idx >= start_page && idx < total_pages);
    assert(memory_map[idx] >= 1);
    memory_map[idx]--;
    if (!memory_map[idx]) {
        free_pages++;
    }
    assert(free_pages > 0 && free_pages < total_pages);
    printk("PUT page 0x%p\n", addr);
}
uint32_t get_cr2() {
    asm volatile("movl %cr2, %eax\n");
}
uint32_t get_cr3() {
    asm volatile("movl %cr3, %eax\n");
}
void set_cr3(uint32_t pde) {
    ASSERT_PAGE(pde);
    asm volatile("movl %%eax, %%cr3\n" ::"a"(pde));
}
// 初始化页表项
static void entry_init(PE* entry, uint32_t index) {
    *(uint32_t*)entry = 0;
    entry->present = 1;
    entry->read_write = 1;
    entry->user_supervisor = 1;
    entry->page_base = index;
}
void mapping_init() {
    PE* pde = (PE*)0x100000;
    memset(pde, 0, PAGE_SIZE);
    uint32_t index = 0;
    for (uint32_t didx = 0; didx < (sizeof(KERNEL_PAGE_TABLE) / 4); didx++) {
        PE* pte = (PE*)KERNEL_PAGE_TABLE[didx];
        memset(pte, 0, PAGE_SIZE);
        PE* dentry = &pde[didx];
        entry_init(dentry, IDX((uint32_t)pte));
        dentry->user_supervisor = 0;
        for (idx_t tidx = 0; tidx < 1024; tidx++, index++) {
            if (index == 0)continue;
            PE* tentry = &pte[tidx];
            entry_init(tentry, index);
            tentry->user_supervisor = 0;
            if (memory_map[index] == 0)free_pages--;
            memory_map[index] = 1;
        }
    }
    PE* entry = &pde[1023];
    entry_init(entry, IDX(pg_base));
}

#endif