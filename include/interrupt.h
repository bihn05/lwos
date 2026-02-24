#ifndef _INTERRUPT_H
#define _INTERRUPT_H

#include <stdint.h>

#pragma pack(push, 1)
// 64位 IDT 表项 (16字节)
typedef struct {
    uint16_t offset_low;    // 目标代码段偏移 0-15
    uint16_t selector;      // 目标代码段选择子 (比如 KERNEL_CS 0x08)
    uint8_t  ist;           // 中断栈表偏移 (Interrupt Stack Table)，填 0 即可
    uint8_t  type_attr;     // 类型与属性标志
    uint16_t offset_mid;    // 目标代码段偏移 16-31
    uint32_t offset_high;   // 目标代码段偏移 32-63
    uint32_t zero;          // 保留位，必须为 0
} idt_entry_t;

// 64位 IDTR 指针
typedef struct {
    uint16_t limit;
    uint64_t base;          // 必须是 64 位指针
} idtr_t;
#pragma pack(pop)

// 传递给 C 语言的中断/异常上下文结构
typedef struct {
    // 1. 我们手动压入的寄存器 (对应汇编里的 push)
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    
    // 2. 中断号与错误码 (我们手动压入的)
    uint64_t int_no;
    uint64_t err_code;
    
    // 3. 硬件 CPU 自动压入的 5 个关键值！
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} int_registers_t;

#endif