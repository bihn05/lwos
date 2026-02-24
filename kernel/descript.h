// kernel/descript.h
#ifndef _GDT_H_
#define _GDT_H_

#include <stdint.h>
#include <string.h>

// 修正：增加到 6 个表项 (0:NULL, 1:K_CS, 2:K_DS, 3:U_CS, 4:U_DS, 5:TSS)
#define GDT_ENTRIES 6

// GDT 访问权限位 (Access Byte)
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

// GDT 标志位 (Flags)
#define GDT_FLAG_32BIT         (1 << 6)
#define GDT_FLAG_4K_GRAN       (1 << 7)

// 标准段选择子
#define NULL_SEG    0x00
#define KERNEL_CS   0x08    // 内核代码段 (DPL=0)
#define KERNEL_DS   0x10    // 内核数据段 (DPL=0)
#define USER_CS     0x1B    // 用户代码段 (DPL=3, 0x18 | 0x03)
#define USER_DS     0x23    // 用户数据段 (DPL=3, 0x20 | 0x03)
#define TSS_SEG     0x28    // 任务状态段 (TSS)

#pragma pack(push, 1)

// GDT 描述符结构 (8字节)
typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} gdt_entry_t;

// GDT 指针结构 (给 lgdt 指令使用)
typedef struct {
    uint16_t limit;
    uint32_t base;
} gdtr_t;

// 任务状态段 (TSS) 结构 (104字节)
typedef struct {
    uint32_t prev_tss;
    uint32_t esp0;      // Ring 0 的栈顶指针 (特权级提升时 CPU 会自动切换到这里)
    uint32_t ss0;       // Ring 0 的栈段选择子
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
    uint32_t es, cs, ss, ds, fs, gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;
} tss_t;

#pragma pack(pop)

// 全局变量声明
gdt_entry_t gdt[GDT_ENTRIES];
tss_t tss;

// 外部汇编函数声明
extern void load_segments(void);

// ---------------- 函数实现 ----------------

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

// 初始化 TSS
void tss_init() {
    memset(&tss, 0, sizeof(tss));
    
    // 当 CPU 从 Ring 3 陷入 Ring 0 (比如时钟中断发生) 时，
    // 会从这里读取内核栈的数据段
    tss.ss0 = KERNEL_DS;
    
    // IOPB (I/O Permission Bitmap) 偏移量指向结构体末尾，表示没有 IOPB
    tss.iomap_base = sizeof(tss);

    // 将 TSS 注册到 GDT 中 (索引 5)
    // 权限位：0x89 (Present, Ring 0, System, 32-bit TSS)
    gdt_set_entry(5, (uint32_t)&tss, sizeof(tss) - 1, 0x89, 0x00);
}

// 加载 GDT 到寄存器
void load_gdt(void) {
    gdtr_t gdtr;
    // 动态计算大小，避免硬编码 48 - 1 的隐患
    gdtr.limit = sizeof(gdt) - 1; 
    gdtr.base = (uint32_t)&gdt;

    __asm volatile("lgdt %0" : : "m"(gdtr));
}

// 供主程序调用的初始化入口
void gdt_init(void) {
    printk("Initializing GDT...\n");

    // 0. 空描述符
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

    // 5. 初始化并加载 TSS
    tss_init();

    // 加载全局描述符表
    load_gdt();

    // 刷新各段寄存器 (cs, ds, es, fs, gs, ss)
    load_segments();

    // 加载任务寄存器 TR (必须在 GDT 和 段寄存器刷新之后进行)
    __asm volatile("ltr %0" : : "r" ((uint16_t)TSS_SEG));

    printk("GDT & TSS initialized successfully.\n");
}

#endif