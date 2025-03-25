#ifndef _KERNEL_MEMORY_H
#define _KERNEL_MEMORY_H

#include <stdint.h>
#include <pristdio.h>

#pragma pack(1)
struct PDE {
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
    uint32_t page_table_base : 20; // 页表基地址（物理地址的高20位）
};
struct PTE {
    uint32_t present : 1;  // 页帧是否存在于内存中
    uint32_t read_write : 1;  // 0 = 只读, 1 = 可读可写
    uint32_t user_supervisor : 1;  // 0 = 超级用户, 1 = 普通用户
    uint32_t write_through : 1;  // 0 = 写回, 1 = 写直达
    uint32_t cache_disable : 1;  // 0 = 启用缓存, 1 = 禁用缓存
    uint32_t accessed : 1;  // 是否被访问过
    uint32_t dirty : 1;  // 是否被修改过
    uint32_t reserved : 1;  // 保留位
    uint32_t global : 1;  // 0 = 非全局, 1 = 全局
    uint32_t available : 3;  // 操作系统可用位
    uint32_t page_frame_base : 20; // 页帧基地址（物理地址的高20位）
};
struct PDE* pg_base = (uint32_t*)0x100000;
uint32_t pd_free() {
    for (int i = 0; i < 1024;) {
        if (pg_base[i].present != 1) {
            return i;
        }
        i++;
    }
    return 0;
}

#endif