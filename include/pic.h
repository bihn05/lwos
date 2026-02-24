#ifndef _PIC_H
#define _PIC_H

#include <stdint.h>
#include <driver/port.h>

// PIC 端口定义
#define PIC1_CMD_PORT  0x20
#define PIC1_DATA_PORT 0x21
#define PIC2_CMD_PORT  0xA0
#define PIC2_DATA_PORT 0xA1

// ICW1 标志位
#define ICW1_ICW4      0x01    // 需要 ICW4
#define ICW1_SINGLE    0x02    // 单级 PIC
#define ICW1_INTERVAL4 0x04    // 调用间隔 4
#define ICW1_LEVEL     0x08    // 电平触发模式
#define ICW1_INIT      0x10    // 初始化命令

// ICW4 标志位
#define ICW4_8086      0x01    // 8086/88 模式
#define ICW4_AUTO      0x02    // 自动 EOI
#define ICW4_BUF_SLAVE 0x08    // 缓冲模式/从 PIC
#define ICW4_BUF_MASTER 0x0C   // 缓冲模式/主 PIC
#define ICW4_SFNM      0x10    // 特殊全嵌套模式

// PIT 端口与 I/O 端口定义
#define PIT_CHANNEL0_DATA 0x40
#define PIT_CHANNEL1_DATA 0x41
#define PIT_CHANNEL2_DATA 0x42
#define PIT_COMMAND_PORT  0x43

// PIT 控制字格式 (Command Word)
#define PIT_CMD_CHANNEL0     0x00 // 选择通道 0
#define PIT_CMD_LOHI         0x30 // 先写低字节，后写高字节 (最常用的读写模式)
#define PIT_CMD_MODE3        0x06 // 方波速率发生器 (最常用的定时模式)
#define PIT_CMD_BINARY       0x00 // 16 位二进制计数模式

// 组合初始化命令
#define PIT_CMD_INIT (PIT_CMD_CHANNEL0 | PIT_CMD_LOHI | PIT_CMD_MODE3 | PIT_CMD_BINARY)

// 定时器基础频率 (1.193182 MHz)
#define PIT_BASE_FREQ 1193182 

// 默认触发频率 (例如 100 Hz，即每 10ms 触发一次)
#define FREQ_T0 1000

void vpic_init(void);
void send_eoi(uint8_t irq_num);
void IRQ_set_mask(uint8_t IRQline);
void IRQ_clear_mask(uint8_t IRQline);
void timer_init(uint32_t frequency);

#endif