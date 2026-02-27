#include <tools.h>

void dump_chunk(void* addr, uint32_t block) {
    // 1. 提取高 32 位和低 32 位地址
    uint64_t full_addr = (uint64_t)addr;
    uint32_t upper_32 = (uint32_t)(full_addr >> 32);
    uint32_t lower_32_base = (uint32_t)(full_addr & 0xfffffff0);
    
    uint8_t* p = (uint8_t*)(full_addr & ~(uint64_t)0xf);

    // 2. 仅在头部打印一次高 32 位地址，节省屏幕空间
    printk("Your upper 32bit address = 0x%08x\n", upper_32);
    printk("Your position (low 32bit) = 0x%08x\n", (uint32_t)full_addr);

    for (uint32_t t = 0; t < block; t++) {
        for (uint32_t i = 0; i < 16; i++) {
            // 3. 计算当前行在低 32 位下的偏移
            uint32_t current_row_low = lower_32_base + (i * 16) + (t * 256);
            printk("%08x: ", current_row_low);

            // 4. 打印 16 字节的十六进制
            for (uint32_t j = 0; j < 16; j++) {
                printk("%02x ", p[i * 16 + j + t * 256]);
            }

            printk("\b|");

            // 5. 打印右半部分 ASCII，增加不可见字符过滤逻辑
            for (uint32_t j = 0; j < 16; j++) {
                uint8_t c = p[i * 16 + j + t * 256];
                // ASCII 可见字符范围是 32 (空格) 到 126 (~)
                if (c >= 32 && c <= 126) {
                    printk("%c", c);
                } else {
                    printk("."); // 不可见字符统一显示为小数点
                }
            }
            printk("\n");
        }
    }
}