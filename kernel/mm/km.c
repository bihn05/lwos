#include <mm/km.h>

// 全局堆指针
chunk_header_t* kheap_first_chunk = 0;

void kheap_init() {
    // 1. 利用 VMM 为堆所在的虚拟地址范围分配物理页并建立映射
    // vmm_alloc_map_region 会自动向 PMM 索要物理页并填入 4 级页表
    vmm_alloc_map_region(KHEAP_START, KHEAP_INITIAL_SIZE, PAGE_PRESENT | PAGE_RW);

    // 2. 初始化第一个超级块
    kheap_first_chunk = (chunk_header_t*)KHEAP_START;
    kheap_first_chunk->next = 0;
    kheap_first_chunk->size = KHEAP_INITIAL_SIZE - sizeof(chunk_header_t);
    kheap_first_chunk->is_free = 1;
    kheap_first_chunk->magic = 0xC0FFEE; // 塞入魔数 (Coffee)
    
    printk("Kernel Heap Initialized at 0x%x%x\n", 
           (uint32_t)(KHEAP_START >> 32), (uint32_t)(KHEAP_START & 0xFFFFFFFF));
}

void* kmalloc(uint64_t size) {
    if (size == 0) return 0; // r u kid me

    // 强制 16 字节对齐
    uint64_t aligned_size = (size + 15) & ~15;

    chunk_header_t* curr = kheap_first_chunk;

    while (curr != 0) {
        // 找到足够大的空闲块
        if (curr->is_free && curr->size >= aligned_size) {
            
            // 是否可以切分？(剩余空间必须够容纳一个新的 header + 至少 16 字节数据)
            if (curr->size >= aligned_size + sizeof(chunk_header_t) + 16) {
                // 确定切分位置
                chunk_header_t* new_chunk = (chunk_header_t*)((uint8_t*)curr + sizeof(chunk_header_t) + aligned_size);

                // 设置新块的属性
                new_chunk->is_free = 1;
                new_chunk->size = curr->size - aligned_size - sizeof(chunk_header_t);
                new_chunk->next = curr->next;
                new_chunk->magic = 0xC0FFEE;

                // 更新当前块属性
                curr->size = aligned_size;
                curr->next = new_chunk;
            }

            // 标记为占用
            curr->is_free = 0;
            return (void*)((uint8_t*)curr + sizeof(chunk_header_t));
        }
        curr = curr->next;
    }
    
    printk("KHEAP OUT OF MEMORY!\n");
    // TODO: 堆空间耗尽，这里可以实现向后动态映射更多物理页的逻辑
    return 0;
}

void kfree(void* ptr) {
    if (ptr == 0) return; // ugnhhhhhhhhhh

    // 找到当前数据的 header
    chunk_header_t* header = (chunk_header_t*)((uint8_t*)ptr - sizeof(chunk_header_t));

    // 触发安全检查机关！
    if (header->magic != 0xC0FFEE) {
        printk("KFREE ERROR: Invalid chunk or corrupted heap! PTR: 0x%x\n", (uint32_t)((uint64_t)ptr & 0xFFFFFFFF));
        return; // 防止搞崩整个链表
    }

    header->is_free = 1;

    // 经典的相邻空闲块合并算法
    chunk_header_t* curr = kheap_first_chunk;

    while (curr != 0 && curr->next != 0) {
        // 如果当前块和下一块都是空闲的
        if (curr->is_free && curr->next->is_free) {
            // 将下一个块的尺寸和它头部的尺寸一并吃掉
            curr->size += sizeof(chunk_header_t) + curr->next->size;
            // 指针跳过下一块
            curr->next = curr->next->next;
        } else {
            curr = curr->next;
        }
    }
}
