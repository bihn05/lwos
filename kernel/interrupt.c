#include <interrupt.h>
#include <printk.h>
#include <tools.h>

// 全局 IDT 表与指针
static idt_entry_t idt[IDT_ENTRIES];
static idtr_t      idt_pointer;

// 引入汇编里自动生成的 256 个函数地址的指针数组！
extern uint64_t isr_stub_table[];
extern void isr_syscall_stub();

// 异常信息字符串数组 (扩展到 32 个，防止越界)
static const char* exception_messages[32] = {
    "#DE - Divide Error", "#DB - Debug Exception", "NMI - Non Maskable Interrupt",
    "#BP - Breakpoint", "#OF - Overflow", "#BR - BOUND Range Exceeded",
    "#UD - Invalid Opcode", "#NM - Device Not Available", "#DF - Double Fault",
    "Coprocessor Segment Overrun", "#TS - Invalid TSS", "#NP - Segment Not Present",
    "#SS - Stack-Segment Fault", "#GP - General Protection", "#PF - Page Fault",
    "Reserved", "#MF - x87 FPU Math Error", "#AC - Alignment Check",
    "#MC - Machine Check", "#XM - SIMD Floating-Point", "#VE - Virtualization",
    "#CP - Control Protection Exception", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved"
};

// 设置 16 字节 IDT 门的“机关”
void idt_set_gate(uint8_t num, uint64_t base, uint16_t sel, uint8_t flags, uint8_t ist) {
    *(uint64_t*)&idt[num] = 0;
    *((uint64_t*)&idt[num] + 1) = 0;
    idt[num].offset_low  = base & 0xFFFF;
    idt[num].selector    = sel;
    idt[num].ist         = ist & 0x07;
    idt[num].type_attr   = flags;
    idt[num].offset_mid  = (base >> 16) & 0xFFFF;
    idt[num].offset_high = (base >> 32) & 0xFFFFFFFF;
    idt[num].zero        = 0;
}

// 极其优雅的 IDT 初始化
void idt_init() {
    // 一个循环搞定 256 个中断门！
    for (int i = 0; i < IDT_ENTRIES; i++) {
        uint8_t flags = IDT_INT_GATE_64;
        
        // 如果你需要用户态 (Ring 3) 能够通过 int 指令调用特定中断 (如软中断)
        if (i == 3 || i == 0x80) {
            flags = IDT_INT_GATE_USER;
        }

        idt_set_gate(i, isr_stub_table[i], KERNEL_CS, flags, 0);
    }

    idt_set_gate(0x20, isr_stub_table[32], KERNEL_CS, IDT_INT_GATE_64, 0);
    idt_set_gate(0x80, (uint64_t)isr_syscall_stub, KERNEL_CS, IDT_INT_GATE_USER, 0);

    idt_pointer.limit = sizeof(idt) - 1;
    idt_pointer.base  = (uint64_t)&idt[0];

    // 直接在 C 语言里使用内联汇编加载 IDTR，省去写一个汇编函数的麻烦
    __asm__ volatile("lidt %0" : : "m"(idt_pointer));
    
    // 初始化 8259A PIC，将硬件中断重映射到 32~47
    vpic_init(); 
    
    // 开启中断
    __asm__ volatile("sti");
}

// 统一的中断与异常分发中心
void interrupt_handler(int_registers_t* regs) {
    uint64_t int_no = regs->int_no;

    // 1. 处理 CPU 异常 (0 ~ 31)
    if (int_no < 32) {
        uint64_t irq_no = int_no - 32;
        // 比如打印蓝屏信息，或者做 Page Fault 的处理
        printk("Exception: %s, Error Code: %x\n", exception_messages[int_no], regs->err_code);
        printk(" Stack : Top = %p, Btm = %p\n", (void*)regs->rsp, (void*)regs->rbp);
        printk(" RIP = 0x%p\n", (void*)regs->rip);
        uint64_t fault_cr2;
        if (int_no == 14) {
            __asm__ volatile ("mov %%cr2, %0" : "=r"(fault_cr2));
            printk("### The instruction at 0x%p referrenced memory at 0x%p. The memory could not be %s, because %s in %s.\n",
                regs->rip, fault_cr2,
                (regs->err_code & 0x2) ? "written" : "read",
                (regs->err_code & 0x1) ? "Page-level protection violation" : "Page not present",
                (regs->err_code & 0x4) ? "User mode" : "Supervisor mode");
        }
        if (int_no == 6) {
            __asm__ volatile ("mov %%cr2, %0" : "=r"(fault_cr2));
            printk("### Invaild Opcode at RIP: 0x%p\n", (void*)regs->rip);
            printk("    Possibly fault address: 0x%p\n", fault_cr2);
            printk("    Current CPU mode: %s\n", (regs->cs & 0x3) ? "User" : "Supervisor");
            //dump_chunk((void*)regs->rip, 1);
        }
        // 如果是严重故障，直接停机
        if (int_no == 6 || int_no == 8 || int_no == 13 || int_no == 14) { 
            while(1) { __asm__ volatile("hlt"); }
        }
    }
    // 2. 处理硬件外设中断 IRQ (32 ~ 47)
    else if (int_no >= 32 && int_no <= 47) {
        uint64_t irq_no = int_no - 32;
        
        // 在这里分发给键盘、时钟等驱动...
        // if (irq_no == 1) keyboard_handler();

        if (irq_no == 0) {
            timer_handler(regs); // 时钟中断
        }
        
        // 处理完必须向 PIC 发送 EOI，否则后续中断将被阻塞
        send_eoi((uint8_t)irq_no);
    }
}

extern video_t video;
extern void schedule();

void timer_handler(int_registers_t* regs) {
    outb(0x20, 0x20);
    schedule();
}