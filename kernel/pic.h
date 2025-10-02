#ifndef _PIC_H
#define _PIC_H

#include <stdint.h>
#include <driver/io.h>
#include <kernel.h>

#define PIC1_CMD_PORT  0x20
#define PIC1_DATA_PORT 0x21
#define PIC2_CMD_PORT  0xA0
#define PIC2_DATA_PORT 0xA1

#define ICW1_ICW4      0x01    // 需要ICW4
#define ICW1_SINGLE    0x02    // 单级PIC
#define ICW1_INTERVAL4 0x04    // 调用间隔4
#define ICW1_LEVEL     0x08    // 电平触发模式
#define ICW1_INIT      0x10    // 初始化命令

#define ICW4_8086      0x01    // 8086/88模式
#define ICW4_AUTO      0x02    // 自动EOI
#define ICW4_BUF_SLAVE 0x08    // 缓冲模式/从PIC
#define ICW4_BUF_MASTER 0x0C   // 缓冲模式/主PIC
#define ICW4_SFNM      0x10    // 特殊全嵌套模式

/**
 * 初始化PIC
 * 将IRQ 0-15 映射到中断向量 32-47
 */
void pic_init(void) {
    // 保存当前的掩码
    uint8_t a1 = inb(PIC1_DATA_PORT);
    uint8_t a2 = inb(PIC2_DATA_PORT);

    // 开始初始化序列
    outb(PIC1_CMD_PORT, ICW1_INIT | ICW1_ICW4);
    outb(PIC2_CMD_PORT, ICW1_INIT | ICW1_ICW4);

    // 设置中断向量偏移
    outb(PIC1_DATA_PORT, 32);  // 主PIC映射到32-39
    outb(PIC2_DATA_PORT, 40);  // 从PIC映射到40-47

    // 告诉主PIC从PIC在IRQ2
    outb(PIC1_DATA_PORT, 4);   // IRQ2连接从PIC
    outb(PIC2_DATA_PORT, 2);   // 从PIC的级联身份

    // 设置8086模式
    outb(PIC1_DATA_PORT, ICW4_8086);
    outb(PIC2_DATA_PORT, ICW4_8086);

    // 恢复保存的掩码
    outb(PIC1_DATA_PORT, a1);
    outb(PIC2_DATA_PORT, a2);

    printk("PIC initialized: IRQ0-7 -> INT 32-39, IRQ8-15 -> INT 40-47\n");
}

/**
 * 发送EOI给PIC
 */
void send_eoi(uint8_t irq_num) {
    if (irq_num >= 8) {
        outb(PIC2_CMD_PORT, 0x20); // 发送EOI到从PIC
    }
    outb(PIC1_CMD_PORT, 0x20);     // 发送EOI到主PIC
}

#endif
