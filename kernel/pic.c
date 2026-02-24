#include <pic.h>

/**
 * 初始化 PIC
 * 将 IRQ 0-15 映射到中断向量 32-47
 */
void vpic_init(void) {
    // 1. 开始初始化序列 (ICW1)
    // 按照 (value, port) 顺序
    outb(ICW1_INIT | ICW1_ICW4, PIC1_CMD_PORT);
    outb(ICW1_INIT | ICW1_ICW4, PIC2_CMD_PORT);
         
    // 2. 设置中断向量偏移 (ICW2)
    outb(32, PIC1_DATA_PORT);  // 主 PIC 映射到 32-39
    outb(40, PIC2_DATA_PORT);  // 从 PIC 映射到 40-47

    // 3. 级联设置 (ICW3)
    outb(4, PIC1_DATA_PORT);   // 主片 IRQ2 连接从片
    outb(2, PIC2_DATA_PORT);   // 从片连接到主片 IRQ2

    // 4. 设置模式 (ICW4)
    outb(ICW4_8086, PIC1_DATA_PORT);
    outb(ICW4_8086, PIC2_DATA_PORT);

    // 5. 默认屏蔽所有硬件中断
    outb(0xFF, PIC1_DATA_PORT);
    outb(0xFF, PIC2_DATA_PORT);
}

/**
 * 发送 EOI (End of Interrupt) 给 PIC
 */
void send_eoi(uint8_t irq_num) {
    if (irq_num >= 8) {
        // (value, port) 顺序
        outb(0x20, PIC2_CMD_PORT); 
    }
    outb(0x20, PIC1_CMD_PORT);
}

/**
 * 屏蔽特定中断位
 */
void IRQ_set_mask(uint8_t IRQline) {
    uint16_t port;
    uint8_t value;

    if(IRQline < 8) {
        port = PIC1_DATA_PORT;
    } else {
        port = PIC2_DATA_PORT;
        IRQline -= 8;
    }
    // 读取当前屏蔽字，修改后再写回
    value = inb(port) | (1 << IRQline);
    outb(value, port);        
}

void timer_init(uint32_t frequency) {
    // 1. 计算分频值
    // 注意：在 64 位下进行除法时，确保操作数类型正确以避免异常
    uint16_t divisor = (uint16_t)(PIT_BASE_FREQ / frequency);

    // 2. 发送控制字
    outb(PIT_CMD_INIT, PIT_COMMAND_PORT);

    // 3. 写入频率分频值 (先低 8 位，后高 8 位)
    outb((uint8_t)(divisor & 0xFF), PIT_CHANNEL0_DATA);
    outb((uint8_t)((divisor >> 8) & 0xFF), PIT_CHANNEL0_DATA);
    
    // 4. 在 PIC 中开启时钟中断屏蔽位 (IRQ 0)
    IRQ_clear_mask(0); 
}
/**
 * 开启特定中断位
 */
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