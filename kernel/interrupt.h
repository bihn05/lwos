#ifndef _INTERRUPT_H_
#define _INTERRUPT_H_

#include <stdint.h>
#include <pic.h>

#include <mem.h>

#define KERNEL_CS 0x08

// IDT 门描述符类型
#define IDT_TASK_GATE		0x05  // 任务门
#define IDT_INT_GATE_16		0x06  // 16位中断门  
#define IDT_TRAP_GATE_16	0x07  // 16位陷阱门
#define IDT_INT_GATE_32		0x0E  // 32位中断门（最常用）
#define IDT_TRAP_GATE_32	0x0F  // 32位陷阱门

// 门属性位
#define IDT_PRESENT		0x80 // 段存在位
#define IDT_DPL_0		0x00 // 描述符特权级 0
#define IDT_DPL_1		0x20 // 描述符特权级 1  
#define IDT_DPL_2		0x40 // 描述符特权级 2
#define IDT_DPL_3		0x60 // 描述符特权级 3

#define IDT_ENTRIES 256

#pragma pack(push, 1)
typedef struct {
	uint16_t base_low;
	uint16_t selector;
	uint8_t zero;
	uint8_t type_attr;
	uint16_t base_high;
} idt_t;
typedef struct {
	uint16_t limit;
	uint32_t base;
} idtr;
typedef struct {
	uint32_t gs;
	uint32_t fs;
	uint32_t es;
	uint32_t ds;

	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t esp;
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;

	uint32_t int_no;
	uint32_t err_code;

	uint32_t eip;
	uint32_t cs;
	uint32_t eflags;
	uint32_t useresp;
	uint32_t ss;
} int_registers_t;
#pragma pack(pop)

idt_t idt[256];
idtr idtptr;

extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr9(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);
extern void isr16(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);

extern void irq0(void);
extern void irq1(void);
extern void irq2(void);
extern void irq3(void);
extern void irq4(void);
extern void irq5(void);
extern void irq6(void);
extern void irq6(void);
extern void irq7(void);
extern void irq8(void);
extern void irq9(void);
extern void irq10(void);
extern void irq11(void);
extern void irq12(void);
extern void irq13(void);
extern void irq14(void);
extern void irq15(void);

void interrupt_handler(int_registers_t* regs);
void irq_handler(int_registers_t* regs);
void default_handler(void);
void reserved_handler(void);

void default_handler(void) {
	printk(" * Unhandled interrupt occurred\n");
	while (1);
}
void reserved_handler(void) {
	printk(" * Reserved interrupt called\n");
	while (1);
}
void interrupt_handler(int_registers_t* regs) {
	printk(" * INTERRUPT #%d\n", regs->int_no);
	uint16_t cs_sel = (uint16_t)(regs->cs & 0xFFFF);
	if (cs_sel != 0x08) {
		printk("USR: SS=0x%02x, ESP=0x%04x\n", (uint16_t)(regs->ss & 0xFFFF), regs->useresp);
	}
	else {
		printk("KRN\n");
	}
}
void irq_handler(int_registers_t* regs) {
	uint8_t irq_num = regs->int_no - 32;
	printk(" * INTERRUPT REQUEST #%d\n", irq_num);
	send_eoi(irq_num);
}
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
	// 参数验证
	if (num >= IDT_ENTRIES) {
		printk("IDT Error: Vector %d out of range (0-%d)\n", num, IDT_ENTRIES - 1);
		return;
	}

	if ((sel & 0xFFFC) == 0) { // 选择子不能为空或无效
		printk("IDT Error: Invalid selector 0x%x for vector %d\n", sel, num);
		return;
	}

	// 设置IDT条目
	idt[num].base_low = base & 0xFFFF;
	idt[num].selector = sel;
	idt[num].zero = 0;
	idt[num].type_attr = flags;
	idt[num].base_high = (base >> 16) & 0xFFFF;
}
void kidt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags, bool out) {
	if (out) {
		printk(" - Interrupt 0x%x, @%x:%x, flags=%x\n", num, sel, base, flags);
	}
	idt_set_gate(num, base, sel, flags);
}
void load_idt(void) {
	idtr idt_pointer;

	idt_pointer.limit = sizeof(idt) - 1;
	idt_pointer.base = (uint32_t)&idt[0];

	__asm volatile("lidt %0" : : "m"(idt_pointer));
	printk("IDT loaded at 0x%x, limit 0x%x\n", idt_pointer.base, idt_pointer.limit);
}
void idt_init() {
	for (int i = 0; i < IDT_ENTRIES; i++) {
		idt_set_gate(i, (uint32_t)default_handler, KERNEL_CS,
			IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	}

	// CPU 异常
	idt_set_gate(0, (uint32_t)isr0, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(1, (uint32_t)isr1, KERNEL_CS, IDT_TRAP_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(2, (uint32_t)isr2, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(3, (uint32_t)isr3, KERNEL_CS, IDT_TRAP_GATE_32 | IDT_PRESENT | IDT_DPL_3); // 断点，用户态可用
	idt_set_gate(4, (uint32_t)isr4, KERNEL_CS, IDT_TRAP_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(5, (uint32_t)isr5, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(6, (uint32_t)isr6, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(7, (uint32_t)isr7, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(8, (uint32_t)isr8, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(9, (uint32_t)isr9, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(10, (uint32_t)isr10, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(11, (uint32_t)isr11, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(12, (uint32_t)isr12, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(13, (uint32_t)isr13, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(14, (uint32_t)isr14, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(15, (uint32_t)isr15, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(16, (uint32_t)isr16, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(17, (uint32_t)isr17, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(18, (uint32_t)isr18, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(19, (uint32_t)isr19, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);

	// 保留中断（20-31）
	for (int i = 20; i <= 31; i++) {
		idt_set_gate(i, (uint32_t)reserved_handler, KERNEL_CS,
			IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	}

	idt_set_gate(32, (uint32_t)irq0, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(33, (uint32_t)irq1, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(34, (uint32_t)irq2, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(35, (uint32_t)irq3, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(36, (uint32_t)irq4, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(37, (uint32_t)irq5, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(38, (uint32_t)irq6, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(39, (uint32_t)irq7, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(40, (uint32_t)irq8, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(41, (uint32_t)irq9, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(42, (uint32_t)irq10, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(43, (uint32_t)irq11, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(44, (uint32_t)irq12, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(45, (uint32_t)irq13, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(46, (uint32_t)irq14, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(47, (uint32_t)irq15, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);

	dump_chunk((uint32_t)&idt, 1);

	load_idt();

	//__asm volatile("sti");
}

// 测试中断0 除〇异常
void d_test_int0(void) {
	__asm volatile(
		"xor %%eax, %%eax\n"    // EAX = 0
		"div %%eax\n"           // 除以0 → 触发异常0
		: : : "eax"
		);
}

#endif