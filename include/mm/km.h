#ifndef _KM_H_
#define _KM_H_

#include <stdint.h>
#include <mm/vmm.h>
#include <mm/pmm.h>

// 64 位下标准的内核高半区堆起始地址
#define KHEAP_START        0xFFFFFFFF82000000ULL
// 物理内存变大了，我们直接把初始堆放大到 8MB
#define KHEAP_INITIAL_SIZE 0x800000 

// 64位块头：刚好 32 字节 (8 * 4)
typedef struct chunk_header {
    struct chunk_header* next;  // 8 bytes
    uint64_t size;              // 8 bytes (当前 chunk 数据区的大小)
    uint64_t is_free;           // 8 bytes
    uint64_t magic;             // 8 bytes (用于安全校验)
} chunk_header_t;

extern chunk_header_t* kheap_first_chunk;

void kheap_init();
void* kmalloc(uint64_t size);
void kfree(void* ptr);
#endif