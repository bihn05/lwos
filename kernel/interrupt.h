#ifndef _INT_H
#define _INT_H

#include <pristdio.h>
#include <port.h>
#include <stdint.h>
#include <global.h>
#include <debug.h>
#include <assert.h>

#define IDT_SIZE 256
#define ENTRY_SIZE 0x30

#define PIC_M_CTRL 0x20
#define PIC_M_DATA 0x21
#define PIC_S_CTRL 0xa0
#define PIC_S_DATA 0xa1
#define PIC_EOI 0x20

#define IRQ_MASTER_NR 0x20
#define IRQ_SLAVE_NR 0x28

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

struct gate_t {
	uint16_t offset0;
	uint16_t selector;
	uint8_t reserved;
	uint8_t type : 4;
	uint8_t segment : 1;
	uint8_t dpl : 2;
	uint8_t present : 1;
	uint16_t offset1;
};

struct gate_t idt[IDT_SIZE];
struct pointer_t idt_ptr;
typedef void* handler_t;
handler_t handler_table[IDT_SIZE];
extern handler_t handler_entry_table[ENTRY_SIZE];
extern void syscall_handler();
extern void page_fault();

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

void send_eoi(int vector) {
	if (vector >= 0x20 && vector < 0x28) {
		outb(PIC_EOI, PIC_M_CTRL);
	}
	if (vector >= 0x28 && vector < 0x30) {
		outb(PIC_EOI, PIC_M_CTRL);
		outb(PIC_EOI, PIC_S_CTRL);
	}
}
void set_exception_handler(uint32_t intr, handler_t handler) {
	assert(intr >= 0 && intr <= 17);
	handler_table[intr] = handler;
}
void set_interrupt_handler(uint32_t irq, handler_t handler) {
	assert(irq >= 0 && irq < 16);
	handler_table[IRQ_MASTER_NR + irq] = handler;
}
void set_interrupt_mask(uint32_t irq, bool enable) {
	assert(irq >= 0 && irq < 16);
	uint16_t port;
	if (irq < 8) {
		port = PIC_M_DATA;
	}
	else {
		port = PIC_S_DATA;
		irq -= 8;
	}
	if (enable) {
		outb(inb(port) & ~(1 << irq), port);
	}
	else {
		outb(inb(port) | (1 << irq), port);
	}
}
bool interrupt_disable() {
	asm volatile(
		"pushfl\n"
		"cli\n"
		"popl %eax\n"
		"shrl $9, %eax\n"
		"andl $1, %eax\n"
		);
}
bool get_interrupt_state() {
	asm volatile(
		"pushfl\n"
		"popl %eax\n"
		"shrl $9, %eax\n"
		"andl $1, %eax\n"
		);
}
void set_interrupt_state(bool state) {
	if (state)
		asm volatile("sti\n");
	else
		asm volatile("cli\n");
}
void default_handler(int vector) {
	send_eoi(vector);
	printk("[%x] default intr called...\n", vector);
}
void exception_handler(
	int vector,
	uint32_t edi, uint32_t esi, uint32_t ebp,
	uint32_t esp, uint32_t ebx, uint32_t edx,
	uint32_t ecx, uint32_t eax, uint32_t gs,
	uint32_t fs, uint32_t es, uint32_t ds,
	uint32_t vector0, uint32_t error, uint32_t eip,
	uint32_t cs, uint32_t eflags) {
	
	char* message = NULL;
	if (vector < 22) {
		message = messages[vector];
	}
	else {
		message = messages[15];
	}

	printk("\nEXCEPTION : %s", message);
	printk("   VECTOR : 0x%02X\n", vector);
	printk("    ERROR : 0x%08X\n", error);
	printk("   EFLAGS : 0x%08X\n", eflags);
	printk("       CS : 0x%02X\n", cs);
	printk("      EIP : 0x%08X\n", eip);
	printk("      ESP : 0x%08X\n", esp);

	while (1);
	return;
}
void pic_init() {
	outb(0x11, PIC_M_CTRL);
	outb(0x20, PIC_M_DATA);
	outb(0x04, PIC_M_DATA);
	outb(0x01, PIC_M_DATA);

	outb(0x11, PIC_S_CTRL);
	outb(0x28, PIC_S_DATA);
	outb(0x02, PIC_S_DATA);
	outb(0x01, PIC_S_DATA);

	outb(0xff, PIC_M_DATA);
	outb(0xff, PIC_S_DATA);
}
void idt_init() {
	for (size_t i = 0; i < IDT_SIZE; i++) {
		struct gate_t* gate = &idt[i];
		handler_t handler = handler_entry_table[i];

		gate->offset0 = (uint32_t)handler & 0xffff;
		gate->offset1 = ((uint32_t)(handler) >>16) & 0xffff;
		gate->selector = 0x08;
		gate->reserved = 0;
		gate->type = 0xe;
		gate->segment = 0;
		gate->dpl = 0;
		gate->present = 1;
	}

	for (size_t i = 0; i < 0x20; i++) {
		handler_table[i] = exception_handler;
	}
	handler_table[0xe] = page_fault;
	for (size_t i = 0x20; i < ENTRY_SIZE; i++) {
		handler_table[i] = default_handler;
	}

	struct gate_t* gate = &idt[0x80];
	gate->offset0 = (uint32_t)syscall_handler & 0xffff;
	gate->offset1 = ((uint32_t)syscall_handler >> 16) & 0xffff;
	gate->selector = 0x08;
	gate->reserved = 0;
	gate->type = 0xe;
	gate->segment = 0;
	gate->dpl = 3;
	gate->present = 1;

	idt_ptr.base = (uint32_t)idt;
	idt_ptr.limit = sizeof(idt) - 1;
	BMB;
	asm volatile("lidt _idt_ptr\n");
}

void interrupt_init() {
	pic_init();
	idt_init();
}

#endif
