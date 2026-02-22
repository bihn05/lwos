#ifndef _FPU_H_
#define _FPU_H_

#include <stdint.h>

// 初始化 FPU 和 SSE 浮点运算支持
void init_fpu() {
    uint32_t cr0;
    uint32_t cr4;

    // 1. 配置 CR0 寄存器，开启原生 x87 FPU
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    
    cr0 &= ~(1 << 2); // 清除 EM (Emulation) 位：告诉 CPU 我们有硬件 FPU，不需要软件模拟
    cr0 |=  (1 << 1); // 设置 MP (Monitor coProcessor) 位：配合 TS 标志位使用
    
    __asm__ volatile("mov %0, %%cr0" :: "r"(cr0));

    // 2. 清除 TS (Task Switched) 标志位
    // 如果 TS 被设置，执行浮点指令会触发异常。在每次任务切换时，操作系统通常会利用这个位来做 FPU 状态的懒加载。
    // 在我们这个简单的阶段，直接清除它。
    __asm__ volatile("clts");

    // 3. 初始化 FPU 状态为默认干净的状态
    __asm__ volatile("fninit");

    // ---------------------------------------------------------
    // 4. 强烈建议：同时开启 SSE/SSE2 支持
    // 因为现代 GCC 编译 printk 中的浮点运算极大概率会生成 SSE 指令
    // ---------------------------------------------------------
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    
    cr4 |= (1 << 9);  // 设置 OSFXSR 位：告诉 CPU 操作系统支持 FXSAVE/FXRSTOR 指令（保存/恢复浮点环境）
    cr4 |= (1 << 10); // 设置 OSXMMEXCPT 位：告诉 CPU 操作系统支持处理 SSE 抛出的无掩码异常
    
    __asm__ volatile("mov %0, %%cr4" :: "r"(cr4));
}

#endif