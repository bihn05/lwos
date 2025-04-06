#ifndef _INTERRUPT_H
#define _INTERRUPT_H

#include <type.h>
#include <port.h>
#include <gd.h>

#define IDT_SIZE 256
#define ENTRY_SIZE 0x30

#define PIC_M_CTRL 0x20 // 主片的控制端口
#define PIC_M_DATA 0x21 // 主片的数据端口
#define PIC_S_CTRL 0xa0 // 从片的控制端口
#define PIC_S_DATA 0xa1 // 从片的数据端口
#define PIC_EOI 0x20    // 通知中断控制器中断结束
#define ENTRY_SIZE 0x30

#define PIC_M_CTRL 0x20 // 主片的控制端口
#define PIC_M_DATA 0x21 // 主片的数据端口
#define PIC_S_CTRL 0xa0 // 从片的控制端口
#define PIC_S_DATA 0xa1 // 从片的数据端口
#define PIC_EOI 0x20    // 通知中断控制器中断结束

#define INTR_DE 0   // 除零错误
#define INTR_DB 1   // 调试
#define INTR_NMI 2  // 不可屏蔽中断
#define INTR_BP 3   // 断点
#define INTR_OF 4   // 溢出
#define INTR_BR 5   // 越界
#define INTR_UD 6   // 指令无效
#define INTR_NM 7   // 协处理器不可用
#define INTR_DF 8   // 双重错误
#define INTR_OVER 9 // 协处理器段超限
#define INTR_TS 10  // 无效任务状态段
#define INTR_NP 11  // 段无效
#define INTR_SS 12  // 栈段错误
#define INTR_GP 13  // 一般性保护异常
#define INTR_PF 14  // 缺页错误
#define INTR_RE1 15 // 保留
#define INTR_MF 16  // 浮点异常
#define INTR_AC 17  // 对齐检测
#define INTR_MC 18  // 机器检测
#define INTR_XM 19  // SIMD 浮点异常
#define INTR_VE 20  // 虚拟化异常
#define INTR_CP 21  // 控制保护异常

#define IRQ_CLOCK 0      // 时钟
#define IRQ_KEYBOARD 1   // 键盘
#define IRQ_CASCADE 2    // 8259 从片控制器
#define IRQ_SERIAL_2 3   // 串口 2
#define IRQ_SERIAL_1 4   // 串口 1
#define IRQ_PARALLEL_2 5 // 并口 2
#define IRQ_SB16 5       // SB16 声卡
#define IRQ_FLOPPY 6     // 软盘控制器
#define IRQ_PARALLEL_1 7 // 并口 1
#define IRQ_RTC 8        // 实时时钟
#define IRQ_REDIRECT 9   // 重定向 IRQ2
#define IRQ_NIC 11       // 网卡
#define IRQ_MOUSE 12     // 鼠标
#define IRQ_MATH 13      // 协处理器 x87
#define IRQ_HARDDISK 14  // ATA 硬盘第一通道
#define IRQ_HARDDISK2 15 // ATA 硬盘第二通道

#define IRQ_MASTER_NR 0x20 // 主片起始向量号
#define IRQ_SLAVE_NR 0x28  // 从片起始向量号

static char* messages[] = {
    "#DE Divide Error\0",
    "#DB RESERVED\0",
    "--  NMI Interrupt\0",
    "#BP Breakpoint\0",
    "#OF Overflow\0",
    "#BR BOUND Range Exceeded\0",
    "#UD Invalid Opcode (Undefined Opcode)\0",
    "#NM Device Not Available (No Math Coprocessor)\0",
    "#DF Double Fault\0",
    "    Coprocessor Segment Overrun (reserved)\0",
    "#TS Invalid TSS\0",
    "#NP Segment Not Present\0",
    "#SS Stack-Segment Fault\0",
    "#GP General Protection\0",
    "#PF Page Fault\0",
    "--  (Intel reserved. Do not use.)\0",
    "#MF x87 FPU Floating-Point Error (Math Fault)\0",
    "#AC Alignment Check\0",
    "#MC Machine Check\0",
    "#XF SIMD Floating-Point Exception\0",
    "#VE Virtualization Exception\0",
    "#CP Control Protection Exception\0",
};

typedef struct gate_t
{
    uint16_t offsetlow;    // 段内偏移 0 ~ 15 位
    uint16_t selector;   // 代码段选择子
    uint8_t reserved;    // 保留不用
    uint8_t type : 4;    // 任务门/中断门/陷阱门
    uint8_t segment : 1; // segment = 0 表示系统段
    uint8_t dpl : 2;     // 使用 int 指令访问的最低权限
    uint8_t present : 1; // 是否有效
    uint16_t offsethigh;    // 段内偏移 16 ~ 31 位
} gate_t;

typedef void* handler_t; // 中断处理函数
gate_t idt[IDT_SIZE];
pointer_t idt_ptr;
handler_t hd_table[IDT_SIZE];
extern handler_t hd_entry_table[ENTRY_SIZE];
void page_fault() {
    while (1);
    return;
}

void send_eoi(int vector)
{
    if (vector >= 0x20 && vector < 0x28)
    {
        outb(PIC_EOI, PIC_M_CTRL);
    }
    if (vector >= 0x28 && vector < 0x30)
    {
        outb(PIC_EOI, PIC_M_CTRL);
        outb(PIC_EOI, PIC_S_CTRL);
    }
}
// 注册异常处理函数
void set_exception_handler(uint32_t intr, handler_t handler) {
    assert(intr >= 0 && intr <= 17);
    hd_table[intr] = handler;
}

// 注册中断处理函数
void set_interrupt_handler(uint32_t irq, handler_t handler) {
    assert(irq >= 0 && irq < 16);
    hd_table[IRQ_MASTER_NR + irq] = handler;
}
void set_interrupt_mask(uint32_t irq, bool enable) {
    assert(irq >= 0 && irq < 16);
    uint16_t port;
    if (irq < 8)
    {
        port = PIC_M_DATA;
    }
    else
    {
        port = PIC_S_DATA;
        irq -= 8;
    }
    if (enable)
    {
        outb(inb(port) & ~(1 << irq), port);
    }
    else
    {
        outb(inb(port) | (1 << irq), port);
    }
}
bool interrupt_disable()
{
    asm volatile(
        "pushfl\n"        // 将当前 eflags 压入栈中
        "cli\n"           // 清除 IF 位，此时外中断已被屏蔽
        "popl %eax\n"     // 将刚才压入的 eflags 弹出到 eax
        "shrl $9, %eax\n" // 将 eax 右移 9 位，得到 IF 位
        "andl $1, %eax\n" // 只需要 IF 位
        );
}

// 获得 IF 位
bool get_interrupt_state()
{
    asm volatile(
        "pushfl\n"        // 将当前 eflags 压入栈中
        "popl %eax\n"     // 将压入的 eflags 弹出到 eax
        "shrl $9, %eax\n" // 将 eax 右移 9 位，得到 IF 位
        "andl $1, %eax\n" // 只需要 IF 位
        );
}

// 设置 IF 位
void set_interrupt_state(bool state)
{
    if (state)
        asm volatile("sti\n");
    else
        asm volatile("cli\n");
}

void default_handler(int vector)
{
    send_eoi(vector);
    printk("[%x] default interrupt called...\n", vector);
}

void exception_handler(
    int vector,
    uint32_t edi, uint32_t esi, uint32_t ebp, uint32_t esp,
    uint32_t ebx, uint32_t edx, uint32_t ecx, uint32_t eax,
    uint32_t gs, uint32_t fs, uint32_t es, uint32_t ds,
    uint32_t vector0, uint32_t error, uint32_t eip, uint32_t cs, uint32_t eflags)
{
    char* message = NULL;
    if (vector < 22)
    {
        message = messages[vector];
    }
    else
    {
        message = messages[15];
    }

    printk("\nEXCEPTION : %s \n", message);
    printk("   VECTOR : 0x%02X\n", vector);
    printk("    ERROR : 0x%08X\n", error);
    printk("   EFLAGS : 0x%08X\n", eflags);
    printk("       CS : 0x%02X\n", cs);
    printk("      EIP : 0x%08X\n", eip);
    printk("      ESP : 0x%08X\n", esp);

    bool hanging = true;

    // 阻塞
    while (hanging)
        ;
    // 通过 EIP 的值应该可以找到出错的位置
    // 也可以在出错时，可以将 hanging 在调试器中手动设置为 0
    // 然后在下面 return 打断点，单步调试，找到出错的位置
    return;
}

// 初始化中断控制器
void pic_init()
{
    outb(0x11, PIC_M_CTRL); // ICW1: 边沿触发, 级联 8259, 需要ICW4.
    outb(0x20, PIC_M_DATA);       // ICW2: 起始中断向量号 0x20
    outb(0x04, PIC_M_DATA); // ICW3: IR2接从片.
    outb(0x01, PIC_M_DATA); // ICW4: 8086模式, 正常EOI
               
    outb(0x11, PIC_S_CTRL); // ICW1: 边沿触发, 级联 8259, 需要ICW4.
    outb(0x28, PIC_S_DATA);       // ICW2: 起始中断向量号 0x28
    outb(0x02, PIC_S_DATA);          // ICW3: 设置从片连接到主片的 IR2 引脚
    outb(0x01, PIC_S_DATA); // ICW4: 8086模式, 正常EOI
               
    outb(0xff, PIC_M_DATA); // 关闭所有中断
    outb(0xff, PIC_S_DATA); // 关闭所有中断

}

// 初始化中断描述符，和中断处理函数数组
void idt_init()
{
    for (size_t i = 0; i < ENTRY_SIZE; i++)
    {
        gate_t* gate = &idt[i];
        handler_t handler = hd_entry_table[i];

        gate->offsetlow = (uint32_t)handler & 0xffff;
        gate->offsethigh = ((uint32_t)handler >> 16) & 0xffff;
        gate->selector = 1 << 3; // 代码段
        gate->reserved = 0;      // 保留不用
        gate->type = 0b1110;     // 中断门
        gate->segment = 0;       // 系统段
        gate->dpl = 0;           // 内核态
        gate->present = 1;       // 有效
    }

    for (size_t i = 0; i < 0x20; i++)
    {
        hd_table[i] = exception_handler;
    }

    hd_table[0xe] = page_fault;

    for (size_t i = 0x20; i < ENTRY_SIZE; i++)
    {
        hd_table[i] = default_handler;
    }

    /*
    // 初始化系统调用
    gate_t* gate = &idt[0x80];
    gate->offsetlow = (uint32_t)syscall_handler & 0xffff;
    gate->offsethigh = ((uint32_t)syscall_handler >> 16) & 0xffff;
    gate->selector = 1 << 3; // 代码段
    gate->reserved = 0;      // 保留不用
    gate->type = 0b1110;     // 中断门
    gate->segment = 0;       // 系统段
    gate->dpl = 3;           // 用户态
    gate->present = 1;       // 有效
    */

    idt_ptr.base = (uint32_t)idt;
    idt_ptr.limit = sizeof(idt) - 1;

    asm volatile("lidt _idt_ptr\n");
}

void interrupt_init()
{
    pic_init();
    idt_init();
}


#endif