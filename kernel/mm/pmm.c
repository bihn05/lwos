#include <mm/pmm.h>

pmm_manager_t pmm_manager;
uint8_t* pmm_bitmap; // 位图指针

void pmm_init() {
    // 获取 loader 阶段探测的内存信息 (假设依然存放在 0x7e00)
    uint32_t ards_count = *(uint32_t*)0x7e00;
    ards_t* ards_buffer = (ards_t*)0x7e10;

    uint64_t max_memory = 0;

    // 计算最大物理内存边界
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

    pmm_manager.total_pages = max_memory / PAGE_SIZE;
    pmm_manager.free_pages = 0;

    // 位图放置在 2MB 处 (0x200000)，避开 0-2MB 的内核与页表区域
    pmm_manager.map_start_addr = 0x200000;
    pmm_bitmap = (uint8_t*)pmm_manager.map_start_addr;

    // 计算位图本身需要的字节数
    uint64_t bitmap_size = pmm_manager.total_pages / 8;

    // 初始化位图：全部置为 1 (已占用)
    for (uint64_t i = 0; i < bitmap_size; i++) {
        pmm_bitmap[i] = 0xff;
    }

    // 根据 ARDS 表，将可用区域清 0
    for (int i = 0; i < ards_count; i++) {

        if (ards_buffer[i].type == ARDS_TYPE_USABLE) {
            uint64_t base = ards_buffer[i].base_addr;
            uint64_t len = ards_buffer[i].length;

            uint64_t start_page = base / PAGE_SIZE;
            uint64_t end_page = (base + len) / PAGE_SIZE;

            for (uint64_t page = start_page; page < end_page && page < pmm_manager.total_pages; page++) {
                pmm_bitmap[page / 8] &= ~(1 << (page % 8));
                pmm_manager.free_pages++;
            }
        }
    }

    // 打印内存大小 (MB)
    printk("Total Memory detected: %d MB\n", (uint32_t)(max_memory / (1024 * 1024)));

    // 保护关键物理内存区域！
    // 0~4KB: 保护 BIOS 数据区
    pmm_reserve_region(0x0, 0x1000);
    // 保护内核代码与数据区 (假设内核加载在 0x10000)
    pmm_reserve_region(0x10000, 0x90000);
    // 保护 VGA 显存区 (文本模式 0xb8000，图形模式 0xa0000 等)
    pmm_reserve_region(0xa0000, 0x60000);
    // 保护 Loader 中建立的 64 位页表区 (0x100000 ~ 0x103000)
    pmm_reserve_region(0x100000, 0x3000);
    // 保护位图自身！
    pmm_reserve_region(0x200000, bitmap_size);

    printk("PMM Init: Bitmap @0x200000, Size=%d bytes, Free Pages: %d\n", (uint32_t)bitmap_size, (uint32_t)pmm_manager.free_pages);
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