
#include <descript.h>
#include <string.h>
gdt_entry_t gdt[GDT_ENTRIES];
// 设置常规 8 字节描述符
void gdt_set_gate(int num, uint8_t access, uint8_t gran) {
    gdt[num].base_low    = 0;
    gdt[num].base_mid    = 0;
    gdt[num].base_high   = 0;
    gdt[num].limit_low   = 0xFFFFF;
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
    // 1. 初始化

    *(uint64_t*)&gdt[0] = 0;
    *(uint64_t*)&gdt[1] = 0x00AF9A000000FFFFULL; // kernel 64-bit code
    *(uint64_t*)&gdt[2] = 0x00AF92000000FFFFULL; // kernel data
    *(uint64_t*)&gdt[3] = 0x00AFFA000000FFFFULL; // user 64-bit code
    *(uint64_t*)&gdt[4] = 0x00AFF2000000FFFFULL; // user data

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
    // 加载GDT
    __asm__ volatile("lgdt %0" : : "m"(gdtr));
    
    // 远跳转刷新CS
    __asm__ volatile(
        "pushq $0x08\n"           // 内核代码段
        "pushq $1f\n"              // 返回地址标签
        "lretq\n"                  // 远返回
        "1:\n"
        "movq $0x10, %%rax\n"      // 内核数据段
        "movq %%rax, %%ds\n"
        "movq %%rax, %%es\n"
        "movq %%rax, %%ss\n"
        "movq %%rax, %%fs\n"
        "movq %%rax, %%gs\n"
        : : : "rax", "memory"
    );
    
    // 加载TSS
    __asm__ volatile("ltr %0" : : "r"((uint16_t)0x28));
    //while (1);
}