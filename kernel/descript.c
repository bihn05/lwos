
#include <descript.h>
#include <string.h>

// 设置常规 8 字节描述符
void gdt_set_gate(int num, uint8_t access, uint8_t gran) {
    gdt[num].base_low    = 0;
    gdt[num].base_mid    = 0;
    gdt[num].base_high   = 0;
    gdt[num].limit_low   = 0;
    gdt[num].access      = access;
    gdt[num].granularity = gran;
}

// 设置 16 字节 TSS 描述符
void gdt_set_tss(int num, uint64_t base, uint32_t limit) {
    gdt_tss_entry_t *tss_desc = (gdt_tss_entry_t *)&gdt[num];
    
    tss_desc->low.limit_low   = limit & 0xFFFF;
    tss_desc->low.base_low    = base & 0xFFFF;
    tss_desc->low.base_mid     = (base >> 16) & 0xFF;
    tss_desc->low.access       = 0x89; // Present, Executable, Accessed (TSS type)
    tss_desc->low.granularity  = (limit >> 16) & 0x0F;
    tss_desc->low.base_high    = (base >> 24) & 0xFF;
    tss_desc->base_upper       = (base >> 32) & 0xFFFFFFFF;
    tss_desc->reserved         = 0;
}

void gdt_tss_init() {
    // 1. 初始化空描述符
    gdt_set_gate(0, 0, 0);

    // 2. 内核代码段: Access=0x9A (P/DPL0/Code/R), Gran=0x20 (L=1, 64-bit)
    gdt_set_gate(1, 0x9A, 0x20);

    // 3. 内核数据段: Access=0x92 (P/DPL0/Data/RW), Gran=0x00
    gdt_set_gate(2, 0x92, 0x00);

    // 4. 用户数据段: Access=0xF2 (P/DPL3/Data/RW), Gran=0x00
    gdt_set_gate(3, 0xF2, 0x00);

    // 5. 用户代码段: Access=0xFA (P/DPL3/Code/R), Gran=0x20 (L=1)
    gdt_set_gate(4, 0xFA, 0x20);

    // 6. 初始化 TSS 结构
    memset(&kernel_tss, 0, sizeof(tss_t));
    kernel_tss.rsp0 = 0x90000; // 设置内核栈顶
    kernel_tss.iomap_base = sizeof(tss_t);

    // 7. 注册 TSS (占用槽位 5 和 6)
    gdt_set_tss(5, (uint64_t)&kernel_tss, sizeof(tss_t) - 1);

    // 8. 加载 GDTR
    gdtr_t gdtr;
    gdtr.limit = sizeof(gdt) - 1;
    gdtr.base  = (uint64_t)&gdt;
    __asm__ volatile("lgdt %0" : : "m"(gdtr));

    load_segments();

    // 9. 加载任务寄存器 TR (选择子 0x28)
    __asm__ volatile("ltr %%ax" : : "a"(0x28));
}