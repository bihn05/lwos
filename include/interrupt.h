#ifndef _INTERRUPT_H_
#define _INTERRUPT_H_

#include <stdint.h>
#include <pic.h>
#define KERNEL_CS 0x08

// 64 位 IDT 门类型标志位
#define IDT_INT_GATE_64     0x8E  // 1000 1110 (Present, DPL=0, 64-bit Interrupt Gate)
#define IDT_TRAP_GATE_64    0x8F  // 1000 1111 (Present, DPL=0, 64-bit Trap Gate)
#define IDT_INT_GATE_USER   0xEE  // 1110 1110 (Present, DPL=3, 允许用户态 int 指令调用)

#define IDT_ENTRIES 256

#pragma pack(push, 1)

// 64 位 IDT 表项 (16 字节) 
typedef struct {
    uint16_t offset_low;    // 目标偏移 0-15
    uint16_t selector;      // 代码段选择子
    uint8_t  ist;           // 中断栈表偏移 (Interrupt Stack Table)，通常填 0
    uint8_t  type_attr;     // 类型和属性标志
    uint16_t offset_mid;    // 目标偏移 16-31
    uint32_t offset_high;   // 目标偏移 32-63
    uint32_t zero;          // 必须为 0
} idt_entry_t;

// 64 位 IDTR 指针
typedef struct {
    uint16_t limit;
    uint64_t base;
} idtr_t;

// 64 位中断上下文寄存器 (与 int.s 中 push 的顺序严格对应)
typedef struct {
    // 手动压入的 15 个通用寄存器
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    
    // 宏里手动压入的
    uint64_t int_no;
    uint64_t err_code;
    
    // 发生中断时 CPU 硬件自动压入的 5 个关键值
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} int_registers_t;

#pragma pack(pop)

// 导出初始化函数
void idt_init(void);
void interrupt_handler(int_registers_t* regs);

void timer_handler(int_registers_t* regs);

#endif