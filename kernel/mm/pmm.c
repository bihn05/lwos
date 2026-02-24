#include <mm/pmm.h>

pmm_manager_t pmm_manager;
uint8_t* pmm_bitmap; // 位图指针

void pmm_init() {
    // 获取 loader 阶段探测的内存信息 (假设依然存放在 0x7e00)
    uint32_t ards_count = *(uint32_t*)0x7e00;
    ards_t* ards_buffer = (ards_t*)0x7e10;

    uint64_t max_memory = 0;

    // 计算最大物理内存边界
    printk("--- Physical Memory Map (ARDS) ---\n");
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
    printk("----------------------------------\n");

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