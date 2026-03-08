#include <mm/pmm.h>

pmm_manager_t pmm_manager;
uint8_t* pmm_bitmap; // 位图指针

void pmm_init(uint64_t kernel_phys_base, uint64_t kernel_phys_size) {
    uint32_t ards_count = *(uint32_t*)0x7e00;
    ards_t* ards_buffer = (ards_t*)0x7e10;

    uint64_t max_memory = 0;

    for (uint32_t i = 0; i < ards_count; i++) {
        uint64_t region_end = ards_buffer[i].base_addr + ards_buffer[i].length;
        if (ards_buffer[i].type == ARDS_TYPE_USABLE && region_end > max_memory) {
            max_memory = region_end;
        }
    }

    pmm_manager.total_pages = (max_memory + PAGE_SIZE - 1) / PAGE_SIZE;
    pmm_manager.free_pages = 0;

    uint64_t bitmap_bytes = (pmm_manager.total_pages + 7) / 8;
    uint64_t bitmap_pages = (bitmap_bytes + PAGE_SIZE - 1) / PAGE_SIZE;

    // 先固定放到 4 MiB 处
    uint64_t bitmap_phys = 0x00400000;
    pmm_manager.map_start_addr = bitmap_phys;
    pmm_bitmap = (uint8_t*)(uintptr_t)bitmap_phys;   // 你当前仍依赖低端 identity map

    // 全部先标记为已占用
    memset(pmm_bitmap, 0xFF, bitmap_pages * PAGE_SIZE);

    // 根据 ARDS 释放 usable 区
    for (uint32_t i = 0; i < ards_count; i++) {
        if (ards_buffer[i].type != ARDS_TYPE_USABLE) continue;

        uint64_t start = ards_buffer[i].base_addr;
        uint64_t end   = ards_buffer[i].base_addr + ards_buffer[i].length;

        uint64_t start_page = start / PAGE_SIZE;
        uint64_t end_page   = end / PAGE_SIZE;   // [start, end)

        for (uint64_t page = start_page; page < end_page && page < pmm_manager.total_pages; page++) {
            uint8_t mask = (uint8_t)(1u << (page % 8));
            if (pmm_bitmap[page / 8] & mask) {
                pmm_bitmap[page / 8] &= (uint8_t)~mask;
                pmm_manager.free_pages++;
            }
        }
    }

    printk("Total Memory detected: %u MB\n", (uint32_t)(max_memory / (1024 * 1024)));

    // 低端 BIOS / 实模式残留
    pmm_reserve_region(0x00000000, 0x001000);
    // VGA / ROM hole
    pmm_reserve_region(0x000A0000, 0x00060000);
    // early stack
    pmm_reserve_region(0x00090000, 0x00001000);
    // early page tables
    pmm_reserve_region(0x00100000, 0x00005000);
    // kernel image
    pmm_reserve_region(kernel_phys_base, kernel_phys_size);
    // pmm bitmap 自身
    pmm_reserve_region(bitmap_phys, bitmap_pages * PAGE_SIZE);

    printk("PMM: total_pages=%u free_pages=%u bitmap=%p bitmap_bytes=%u\n",
           (uint32_t)pmm_manager.total_pages,
           (uint32_t)pmm_manager.free_pages,
           pmm_bitmap,
           (uint32_t)bitmap_bytes);
}

void pmm_reserve_region(uint64_t start_addr, uint64_t size) {
    uint64_t start_page = start_addr / PAGE_SIZE;
    uint64_t end_page = (start_addr + size) / PAGE_SIZE;
    if ((start_addr + size) % PAGE_SIZE != 0) end_page++;

    for (uint64_t i = start_page; i < end_page; i++) {
        if (i < pmm_manager.total_pages) {
            if ((pmm_bitmap[i / 8] & (1 << (i % 8))) == 0) {
                pmm_bitmap[i / 8] |= (1 << (i % 8));
                pmm_manager.free_pages--;
            }
        }
    }
}

void pmm_reserve_page(uint64_t addr) {
    pmm_reserve_region(addr, PAGE_SIZE);
}

uint64_t pmm_alloc_page() {
    for (uint64_t i = 0; i < pmm_manager.total_pages; i++) {
        if (!(pmm_bitmap[i / 8] & (1 << (i % 8)))) {
            pmm_bitmap[i / 8] |= (1 << (i % 8));
            pmm_manager.free_pages--;
            return i * PAGE_SIZE; // 返回 64 位物理地址
        }
    }
    printk("PMM: OUT OF MEMORY!\n");
    return 0;
}

void pmm_free_page(uint64_t phy_addr) {
    uint64_t page_index = phy_addr / PAGE_SIZE;

    if (page_index < pmm_manager.total_pages) {
        if (pmm_bitmap[page_index / 8] & (1 << (page_index % 8))) {
            pmm_bitmap[page_index / 8] &= ~(1 << (page_index % 8));
            pmm_manager.free_pages++;
        }
    }
}

void print_pmm_info() {
    uint32_t ards_count = *(uint32_t*)0x7e00;
    ards_t* ards_buffer = (ards_t*)0x7e10;

    uint64_t max_memory = 0;

    for (int i = 0; i < ards_count; i++) {
        // 打印每一项的详情
        // %x 打印 32 位，我们将 64 位拆开显示以确保在实体机能看全
        uint32_t base_low = (uint32_t)(ards_buffer[i].base_addr & 0xFFFFFFFF);
        uint32_t base_high = (uint32_t)(ards_buffer[i].base_addr >> 32);
        uint32_t len_low = (uint32_t)(ards_buffer[i].length & 0xFFFFFFFF);
        uint32_t type = ards_buffer[i].type;

        printk("[%02d] Base:0x%08x%08x Len:0x%08x Type:%d\n", 
               i, base_high, base_low, len_low, type);

        uint64_t region_end = ards_buffer[i].base_addr + ards_buffer[i].length;
        if (ards_buffer[i].type == ARDS_TYPE_USABLE && region_end > max_memory) {
            max_memory = region_end;
        }
    }


    printk("Total Memory detected: %d MB\n", (uint32_t)(max_memory / (1024 * 1024)));
}
// 分配 count 个连续的物理页 (用于 DMA 或大块内存)
// 返回值: 连续物理内存的首地址 (物理地址)，失败返回 0
uint64_t pmm_alloc_contiguous_pages(uint64_t count) {
    if (count == 0) return 0;

    uint64_t start_page = 0;
    uint64_t contiguous_free = 0;

    // 遍历所有物理页，寻找连续的 count 个空闲页
    // 提示: 为了避开低地址历史遗留问题，你也可以让 i 从 4096 (即 16MB) 开始
    for (uint64_t i = 0; i < pmm_manager.total_pages; i++) {
        
        // 检查当前页是否空闲 (位图对应位为 0)
        if (!(pmm_bitmap[i / 8] & (1 << (i % 8)))) {
            if (contiguous_free == 0) {
                start_page = i; // 记录这块连续区域的起点
            }
            contiguous_free++;

            // 找够了目标数量！
            if (contiguous_free == count) {
                // 1. 将这些页在位图中标记为已占用
                for (uint64_t j = start_page; j < start_page + count; j++) {
                    pmm_bitmap[j / 8] |= (1 << (j % 8));
                }
                // 2. 更新系统空闲页统计
                pmm_manager.free_pages -= count;
                
                // 3. 返回真实的物理基地址
                return start_page * PAGE_SIZE; 
            }
        } else {
            // 遇到被占用的页，连续性被打破，计数器清零，重新开始寻找
            contiguous_free = 0;
        }
    }

    printk("PMM: OUT OF CONTIGUOUS MEMORY (Requested %d pages)!\n", (uint32_t)count);
    return 0; // 失败返回 0
}

// 释放 count 个连续的物理页
void pmm_free_contiguous_pages(uint64_t phy_addr, uint64_t count) {
    if (phy_addr % PAGE_SIZE != 0 || count == 0) return;

    uint64_t start_page = phy_addr / PAGE_SIZE;

    for (uint64_t i = start_page; i < start_page + count; i++) {
        if (i < pmm_manager.total_pages) {
            // 如果该页是占用状态，则释放它
            if (pmm_bitmap[i / 8] & (1 << (i % 8))) {
                pmm_bitmap[i / 8] &= ~(1 << (i % 8));
                pmm_manager.free_pages++;
            }
        }
    }
}