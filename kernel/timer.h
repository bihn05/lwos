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
#define PIT_CMD_CHANNEL0     (0 << 6) // 选择通道 0
#define PIT_CMD_LOHI         (3 << 4) // 先写低字节，再写高字节 (访问模式)
#define PIT_CMD_MODE3        (3 << 1) // 方波发生器模式（最常用的定时模式）
#define PIT_CMD_BINARY       (0 << 0) // 16位二进制模式

// 组合命令字
#define PIT_CMD_INIT (PIT_CMD_CHANNEL0 | PIT_CMD_LOHI | PIT_CMD_MODE3 | PIT_CMD_BINARY)

// 计算周期重载
#define PIT_BASE_FREQ 1193182 // 计时器输入频率

#define FREQ_T0 1 // 100 Hz
void init_pit(void) {
	uint16_t reload_value = PIT_BASE_FREQ / FREQ_T0;

	outb(reload_value & 0xFF, PIT_CHANNEL0_DATA);
	outb((reload_value >> 8) & 0xFF, PIT_CHANNEL0_DATA);

	printk(" - PIT for %d Hz\n", FREQ_T0);
}

#endif