// gdt.h
#ifndef GDT_H
#define GDT_H

#include <stdint.h>

#define GDT_ENTRIES 5

// GDT 访问权限位
#define GDT_ACCESS_PRESENT      0x80
#define GDT_ACCESS_RING0        0x00
#define GDT_ACCESS_RING1        0x20
#define GDT_ACCESS_RING2        0x40
#define GDT_ACCESS_RING3        0x60
#define GDT_ACCESS_SYS          0x00
#define GDT_ACCESS_NONSYS       0x10
#define GPT_TYPE_CODE           0x08
#define GPT_TYPE_DATA           0x00
#define GPT_TYPE_EXPAND         0x04
#define GPT_TYPE_RW             0x02
#define GPT_TYPE_RO             0x00
#define GPT_TYPE_ACCESS         0x01

// GDT 标志位
#define GDT_FLAG_32BIT         (1 << 6)
#define GDT_FLAG_4K_GRAN       (1 << 7)

// 标准段选择子
#define NULL_SEG    0x00
#define KERNEL_CS   0x08    // 内核代码段
#define KERNEL_DS   0x10    // 内核数据段
#define USER_CS     0x1B    // 用户代码段 (DPL=3)
#define USER_DS     0x23    // 用户数据段 (DPL=3)
#define TSS_SEG     0x28    // TSS 段

// GDT 描述符结构
#pragma pack(push, 1)
typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} gdt_entry_t;
// GDT 指针结构
typedef struct {
    uint16_t limit;
    uint32_t base;
} gdtr_t;
#pragma pack(pop)

// 函数声明
void gdt_init(void);
void gdt_set_entry(int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);
void load_gdt(void);

extern void load_segments(void);  // 汇编函数

gdt_entry_t gdt[GDT_ENTRIES];
void gdt_set_entry(int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    if (index >= GDT_ENTRIES) {
        printk("GDT Error: Index %d out of bounds\n", index);
        return;
    }

    gdt[index].limit_low = limit & 0xFFFF;
    gdt[index].base_low = base & 0xFFFF;
    gdt[index].base_middle = (base >> 16) & 0xFF;
    gdt[index].access = access;
    gdt[index].granularity = (limit >> 16) & 0x0F;
    gdt[index].granularity |= gran & 0xF0;
    gdt[index].base_high = (base >> 24) & 0xFF;
}

void gdt_init(void) {
    printk("Initializing GDT...\n");

    // 0. 空描述符（必须为0）
    gdt_set_entry(0, 0, 0, 0, 0);

    // 1. 内核代码段 (Ring 0)
    gdt_set_entry(1, 0, 0xFFFFF,
        GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_NONSYS | GPT_TYPE_CODE | GPT_TYPE_RW,
        GDT_FLAG_32BIT | GDT_FLAG_4K_GRAN);

    // 2. 内核数据段 (Ring 0)
    gdt_set_entry(2, 0, 0xFFFFF,
        GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_NONSYS | GPT_TYPE_DATA | GPT_TYPE_RW,
        GDT_FLAG_32BIT | GDT_FLAG_4K_GRAN);

    // 3. 用户代码段 (Ring 3)
    gdt_set_entry(3, 0, 0xFFFFF,
        GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_NONSYS | GPT_TYPE_CODE | GPT_TYPE_RW,
        GDT_FLAG_32BIT | GDT_FLAG_4K_GRAN);

    // 4. 用户数据段 (Ring 3)
    gdt_set_entry(4, 0, 0xFFFFF,
        GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_NONSYS | GPT_TYPE_DATA | GPT_TYPE_RW,
        GDT_FLAG_32BIT | GDT_FLAG_4K_GRAN);

    // 加载GDT
    load_gdt();

    // 重新加载段寄存器
    load_segments();

    printk("GDT initialized successfully.\n");
    printk("Kernel CS: 0x%02x, Kernel DS: 0x%02x\n", KERNEL_CS, KERNEL_DS);
    printk("User CS: 0x%02x, User DS: 0x%02x\n", USER_CS, USER_DS);
}

void load_gdt(void) {
    gdtr_t gdtr;
    gdtr.limit = 48 - 1;
    gdtr.base = (uint32_t)&gdt;

    __asm volatile("lgdt %0" : : "m"(gdtr));
}

#endif