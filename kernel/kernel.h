#ifndef _KERNEL_H
#define _KERNEL_H

#include <stdint.h>
#include <stdarg.h>
#include <driver/video.h>
#include <vsprintf.h>

// 安全关闭中断，并返回关闭前的 EFLAGS 状态
static inline uint32_t cli_and_save() {
    uint32_t flags;
    __asm__ volatile(
        "pushfl \n\t"      // 32位使用 pushfl
        "pop %0 \n\t"
        "cli \n\t"
        : "=r"(flags) : : "memory"
    );
    return flags;
}

// 恢复 EFLAGS 状态
static inline void restore_flags(uint32_t flags) {
    __asm__ volatile(
        "push %0 \n\t"
        "popfl \n\t"      // 32位使用 popfl
        : : "r"(flags) : "memory"
    );
}

static char ch_buffer[1024];;
int printk(const char* format, ...) {
	uint32_t flags;
    asm volatile("pushfl; pop %0; cli" : "=r"(flags));

	va_list args;
	va_start(args, format);
	int len = vsprintf(ch_buffer, format, args);
	va_end(args);
	for (int i = 0; i < len; i++) {
		putchar(ch_buffer[i]);
	}

	asm volatile("push %0; popfl" : : "r"(flags));

	return len;
}

#endif