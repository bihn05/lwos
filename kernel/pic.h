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
void vpic_init(void) {
    // 开始初始化序列
    outb(ICW1_INIT | ICW1_ICW4, PIC1_CMD_PORT);
    outb(ICW1_INIT | ICW1_ICW4, PIC2_CMD_PORT);
         
    // 设置中断向量偏移
    outb(32, PIC1_DATA_PORT);  // 主PIC映射到32-39
    outb(40, PIC2_DATA_PORT);  // 从PIC映射到40-47

    // 告诉主PIC从PIC在IRQ2
    outb(4, PIC1_DATA_PORT);   // IRQ2连接从PIC
    outb(2, PIC2_DATA_PORT);   // 从PIC的级联身份

    // 设置8086模式
    outb(ICW4_8086, PIC1_DATA_PORT);
    outb(ICW4_8086, PIC2_DATA_PORT);

    outb(0xff, PIC1_DATA_PORT);
    outb(0xff, PIC2_DATA_PORT);

    printk("PIC initialized: IRQ0-7 -> INT 32-39, IRQ8-15 -> INT 40-47\n");
}

/**
 * 发送EOI给PIC
 */
void send_eoi(uint8_t irq_num) {
    if (irq_num >= 8) {
        outb(0x20, PIC2_CMD_PORT); // 发送EOI到从PIC
    }
    outb(0x20, PIC1_CMD_PORT);     // 发送EOI到主PIC
}
void IRQ_set_mask(uint8_t IRQline) {
    uint16_t port;
    uint8_t value;

    if(IRQline < 8) {
        port = PIC1_DATA_PORT;
    } else {
        port = PIC2_DATA_PORT;
        IRQline -= 8;
    }
    value = inb(port) | (1 << IRQline);
    outb(value, port);        
}
void IRQ_clear_mask(uint8_t IRQline) {
    uint16_t port;
    uint8_t value;

    if (IRQline < 8) {
        port = PIC1_DATA_PORT;
    }
    else {
        port = PIC2_DATA_PORT;
        IRQline -= 8;
    }
    value = inb(port) & ~(1 << IRQline);
    outb(value, port);
}

#endif
