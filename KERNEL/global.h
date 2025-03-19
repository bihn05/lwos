#ifndef _GLOBAL_DESC_H
#define _GLOBAL_DESC_H

#include <stdint.h> 
#include <string.h>

#define GDT_SIZE 128

#define KERNEL_CODE_IDX 1
#define KERNEL_DATA_IDX 2
#define KERNEL_TSS_IDX 3

#define USER_CODE_IDX 4
#define USER_DATA_IDX 5

#define KERNEL_CODE_SELECTOR (KERNEL_CODE_IDX << 3)
#define KERNEL_DATA_SELECTOR (KERNEL_DATA_IDX << 3)
#define KERNEL_TSS_SELECTOR (KERNEL_TSS_IDX << 3)

#define USER_CODE_SELECTOR (USER_CODE_IDX << 3 | 0b11)
#define USER_DATA_SELECTOR (USER_DATA_IDX << 3 | 0b11)

typedef struct TSS {
    uint32_t lastlink; // 前一个任务的链接，保存了前一个任状态段的段选择子
    uint32_t esp0;     // ring0 的栈顶地址
    uint32_t ss0;      // ring0 的栈段选择子
    uint32_t esp1;     // ring1 的栈顶地址
    uint32_t ss1;      // ring1 的栈段选择子
    uint32_t esp2;     // ring2 的栈顶地址
    uint32_t ss2;      // ring2 的栈段选择子
    uint32_t cr3;
    uint32_t eip;
    uint32_t flags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldtr;          // 局部描述符选择子
    uint16_t trace : 1;     // 如果置位，任务切换时将引发一个调试异常
    uint16_t reversed : 15; // 保留不用
    uint16_t iobase;        // I/O 位图基地址，16 位从 TSS 到 IO 权限位图的偏移
    uint32_t ssp;           // 任务影子栈指针
} _packed TSS;

uint64_t gdt[GDT_SIZE];
uint64_t pointer;
TSS tss;

//Init Descriptor but limit grnd 4kb
void InitDescriptor(uint64_t* desc, uint32_t base, uint32_t limit) {
    uint64_t tmp1, tmp2;
    tmp1 = base & 0xff000000;
    tmp2 = limit & 0xf0000;
    tmp1 = tmp1 + tmp2;
    tmp2 = base & 0xff0000;
    tmp2 = tmp2 >> 16;
    tmp1 = tmp1 + tmp2;
    tmp1 = tmp1 << 16;
    tmp2 = base & 0xffff;
    tmp1 = tmp1 + tmp2;
    tmp1 = tmp1 << 16;
    tmp2 = limit & 0xffff;
    tmp1 = tmp1 + tmp2;
    *desc = tmp1;
}
void InitGDT() {
    asm volatile("lgdt %0" : : "m"(pointer));
}

void InitTSS() {
    asm volatile(
        "ltr %%ax\n" ::"a"(KERNEL_TSS_SELECTOR));
}
#endif