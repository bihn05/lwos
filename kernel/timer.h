#ifndef _TIMER_H
#define _TIMER_H

#include <stdint.h>
#include <driver/io.h>
#include <kernel.h>

// PIT 的 I/O 端口
#define PIT_CHANNEL0_DATA 0x40
#define PIT_CHANNEL1_DATA 0x41
#define PIT_CHANNEL2_DATA 0x42
#define PIT_COMMAND_PORT  0x43

// PIT 命令字格式
#define PIT_CMD_CHANNEL0     0x00 // 选择通道 0
#define PIT_CMD_LOHI         0x30 // 先写低字节，再写高字节 (访问模式)
#define PIT_CMD_MODE3        0x06 // 方波发生器模式（最常用的定时模式）
#define PIT_CMD_BINARY       0x00 // 16位二进制模式

// 组合命令字
#define PIT_CMD_INIT (PIT_CMD_CHANNEL0 | PIT_CMD_LOHI | PIT_CMD_MODE3 | PIT_CMD_BINARY)

// 计算周期重载
#define PIT_BASE_FREQ 1193182 // 计时器输入频率

#define FREQ_T0 1 // 100 Hz
int intcnt = 0;
extern void timer_hd(void);
void timer_handler(void) {
	intcnt++;

	if (intcnt % 1 == 0) {
		putchar('A');
	}
	outb(0x20, 0x20);
}
void pit_init(void) {
	uint16_t reload_value = PIT_BASE_FREQ / FREQ_T0;

	idt_set_gate(0x20, (uint32_t)timer_hd, 0x08, 0x8e);
	idt_flush();

	outb(PIT_CMD_INIT, PIT_COMMAND_PORT);
	outb(reload_value & 0xFF, PIT_CHANNEL0_DATA);
	outb((reload_value >> 8) & 0xFF, PIT_CHANNEL0_DATA);

	IRQ_clear_mask(0);
	printk(" - PIT for %d Hz, reload %x\n", FREQ_T0, reload_value);
}

#endif