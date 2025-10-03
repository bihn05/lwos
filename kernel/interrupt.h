#ifndef _INTERRUPT_H_
#define _INTERRUPT_H_

#include <stdint.h>
#include <pic.h>
#include <debug.h>
#include <isr.h>

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
char msg[][40] = {
	"#DE - Divide Error\0",
	"#DB - Debug Exception\0",
	"NMI - NMI Interrupt\0",
	"#BP - Breakpoint\0",
	"#OF - Overflow\0",
	"#BR - BOUND Range Exceeded\0",
	"#UD - Invalid Opcode\0",
	"#NM - No Math Coprocessor\0",
	"#DF - Double Fault\0",
	"N/A - Coprocessor Segment Overrun\0",
	"#TS - Double Fault\0",
	"#NP - Segment Not Present\0",
	"#SS - Stack-Segment Fault\0",
	"#GP - General Protection\0",
	"#PF - Page Fault\0",
	"N/A - Intel Reserved\0",
	"#MF - Math Fault\0",
	"#AC - Alignment Check\0",
	"#MC - Machine Check\0",
	"#XM - SIMD Floating-Point Exception\0",
	"#VE - Vitualization Exception\0",
	"#CP - Control Protection Exception\0",
};
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
extern void pic_init(void);
void default_handler(void) {
	printk(" * Unhandled interrupt occurred\n");
	bxbp();
	while (1);
}
void reserved_handler(void) {
	printk(" * Reserved interrupt called\n");
	while (1);
}
void interrupt_handler(int_registers_t* regs) {
	printk(" * INTERRUPT #%d", regs->int_no);
	uint16_t cs_sel = (uint16_t)(regs->cs & 0xFFFF);
	printk("%s  ", msg[regs->int_no]);
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
extern void idt_flush(void);
idtr idt_pointer;
void load_idt(void) {
	idt_pointer.limit = 256 * sizeof(idt_t) - 1;
	idt_pointer.base = (uint32_t)&idt[0];
	idt_flush();
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
	idt_set_gate(20, (uint32_t)isr19, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(21, (uint32_t)isr19, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);

	// 保留中断（20-31）
	for (int i = 22; i <= 31; i++) {
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

	idt_set_gate(48, (uint32_t)unhandle_int48, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(49, (uint32_t)unhandle_int49, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(50, (uint32_t)unhandle_int50, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(51, (uint32_t)unhandle_int51, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(52, (uint32_t)unhandle_int52, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(53, (uint32_t)unhandle_int53, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(54, (uint32_t)unhandle_int54, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(55, (uint32_t)unhandle_int55, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(56, (uint32_t)unhandle_int56, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(57, (uint32_t)unhandle_int57, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(58, (uint32_t)unhandle_int58, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(59, (uint32_t)unhandle_int59, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(60, (uint32_t)unhandle_int60, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(61, (uint32_t)unhandle_int61, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(62, (uint32_t)unhandle_int62, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(63, (uint32_t)unhandle_int63, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(64, (uint32_t)unhandle_int64, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(65, (uint32_t)unhandle_int65, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(66, (uint32_t)unhandle_int66, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(67, (uint32_t)unhandle_int67, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(68, (uint32_t)unhandle_int68, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(69, (uint32_t)unhandle_int69, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(70, (uint32_t)unhandle_int70, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(71, (uint32_t)unhandle_int71, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(72, (uint32_t)unhandle_int72, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(73, (uint32_t)unhandle_int73, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(74, (uint32_t)unhandle_int74, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(75, (uint32_t)unhandle_int75, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(76, (uint32_t)unhandle_int76, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(77, (uint32_t)unhandle_int77, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(78, (uint32_t)unhandle_int78, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(79, (uint32_t)unhandle_int79, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(80, (uint32_t)unhandle_int80, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(81, (uint32_t)unhandle_int81, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(82, (uint32_t)unhandle_int82, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(83, (uint32_t)unhandle_int83, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(84, (uint32_t)unhandle_int84, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(85, (uint32_t)unhandle_int85, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(86, (uint32_t)unhandle_int86, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(87, (uint32_t)unhandle_int87, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(88, (uint32_t)unhandle_int88, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(89, (uint32_t)unhandle_int89, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(90, (uint32_t)unhandle_int90, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(91, (uint32_t)unhandle_int91, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(92, (uint32_t)unhandle_int92, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(93, (uint32_t)unhandle_int93, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(94, (uint32_t)unhandle_int94, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(95, (uint32_t)unhandle_int95, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(96, (uint32_t)unhandle_int96, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(97, (uint32_t)unhandle_int97, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(98, (uint32_t)unhandle_int98, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(99, (uint32_t)unhandle_int99, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(100, (uint32_t)unhandle_int100, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(101, (uint32_t)unhandle_int101, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(102, (uint32_t)unhandle_int102, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(103, (uint32_t)unhandle_int103, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(104, (uint32_t)unhandle_int104, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(105, (uint32_t)unhandle_int105, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(106, (uint32_t)unhandle_int106, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(107, (uint32_t)unhandle_int107, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(108, (uint32_t)unhandle_int108, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(109, (uint32_t)unhandle_int109, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(110, (uint32_t)unhandle_int110, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(111, (uint32_t)unhandle_int111, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(112, (uint32_t)unhandle_int112, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(113, (uint32_t)unhandle_int113, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(114, (uint32_t)unhandle_int114, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(115, (uint32_t)unhandle_int115, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(116, (uint32_t)unhandle_int116, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(117, (uint32_t)unhandle_int117, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(118, (uint32_t)unhandle_int118, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(119, (uint32_t)unhandle_int119, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(120, (uint32_t)unhandle_int120, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(121, (uint32_t)unhandle_int121, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(122, (uint32_t)unhandle_int122, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(123, (uint32_t)unhandle_int123, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(124, (uint32_t)unhandle_int124, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(125, (uint32_t)unhandle_int125, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(126, (uint32_t)unhandle_int126, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(127, (uint32_t)unhandle_int127, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(128, (uint32_t)unhandle_int128, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(129, (uint32_t)unhandle_int129, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(130, (uint32_t)unhandle_int130, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(131, (uint32_t)unhandle_int131, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(132, (uint32_t)unhandle_int132, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(133, (uint32_t)unhandle_int133, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(134, (uint32_t)unhandle_int134, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(135, (uint32_t)unhandle_int135, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(136, (uint32_t)unhandle_int136, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(137, (uint32_t)unhandle_int137, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(138, (uint32_t)unhandle_int138, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(139, (uint32_t)unhandle_int139, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(140, (uint32_t)unhandle_int140, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(141, (uint32_t)unhandle_int141, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(142, (uint32_t)unhandle_int142, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(143, (uint32_t)unhandle_int143, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(144, (uint32_t)unhandle_int144, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(145, (uint32_t)unhandle_int145, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(146, (uint32_t)unhandle_int146, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(147, (uint32_t)unhandle_int147, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(148, (uint32_t)unhandle_int148, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(149, (uint32_t)unhandle_int149, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(148, (uint32_t)unhandle_int148, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(149, (uint32_t)unhandle_int149, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(150, (uint32_t)unhandle_int150, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(151, (uint32_t)unhandle_int151, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(152, (uint32_t)unhandle_int152, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(153, (uint32_t)unhandle_int153, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(154, (uint32_t)unhandle_int154, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(155, (uint32_t)unhandle_int155, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(156, (uint32_t)unhandle_int156, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(157, (uint32_t)unhandle_int157, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(158, (uint32_t)unhandle_int158, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(159, (uint32_t)unhandle_int159, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(160, (uint32_t)unhandle_int160, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(161, (uint32_t)unhandle_int161, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(162, (uint32_t)unhandle_int162, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(163, (uint32_t)unhandle_int163, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(164, (uint32_t)unhandle_int164, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(165, (uint32_t)unhandle_int165, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(166, (uint32_t)unhandle_int166, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(167, (uint32_t)unhandle_int167, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(168, (uint32_t)unhandle_int168, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(169, (uint32_t)unhandle_int169, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(170, (uint32_t)unhandle_int170, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(171, (uint32_t)unhandle_int171, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(172, (uint32_t)unhandle_int172, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(173, (uint32_t)unhandle_int173, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(174, (uint32_t)unhandle_int174, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(175, (uint32_t)unhandle_int175, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(176, (uint32_t)unhandle_int176, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(177, (uint32_t)unhandle_int177, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(178, (uint32_t)unhandle_int178, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(179, (uint32_t)unhandle_int179, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(180, (uint32_t)unhandle_int180, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(181, (uint32_t)unhandle_int181, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(182, (uint32_t)unhandle_int182, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(183, (uint32_t)unhandle_int183, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(184, (uint32_t)unhandle_int184, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(185, (uint32_t)unhandle_int185, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(186, (uint32_t)unhandle_int186, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(187, (uint32_t)unhandle_int187, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(188, (uint32_t)unhandle_int188, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(189, (uint32_t)unhandle_int189, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(190, (uint32_t)unhandle_int190, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(191, (uint32_t)unhandle_int191, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(192, (uint32_t)unhandle_int192, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(193, (uint32_t)unhandle_int193, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(194, (uint32_t)unhandle_int194, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(195, (uint32_t)unhandle_int195, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(196, (uint32_t)unhandle_int196, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(197, (uint32_t)unhandle_int197, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(198, (uint32_t)unhandle_int198, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(199, (uint32_t)unhandle_int199, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(200, (uint32_t)unhandle_int200, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(201, (uint32_t)unhandle_int201, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(202, (uint32_t)unhandle_int202, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(203, (uint32_t)unhandle_int203, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(204, (uint32_t)unhandle_int204, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(205, (uint32_t)unhandle_int205, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(206, (uint32_t)unhandle_int206, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(207, (uint32_t)unhandle_int207, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(208, (uint32_t)unhandle_int208, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(209, (uint32_t)unhandle_int209, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(210, (uint32_t)unhandle_int210, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(211, (uint32_t)unhandle_int211, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(212, (uint32_t)unhandle_int212, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(213, (uint32_t)unhandle_int213, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(214, (uint32_t)unhandle_int214, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(215, (uint32_t)unhandle_int215, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(216, (uint32_t)unhandle_int216, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(217, (uint32_t)unhandle_int217, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(218, (uint32_t)unhandle_int218, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(219, (uint32_t)unhandle_int219, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(220, (uint32_t)unhandle_int220, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(221, (uint32_t)unhandle_int221, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(222, (uint32_t)unhandle_int222, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(223, (uint32_t)unhandle_int223, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(224, (uint32_t)unhandle_int224, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(225, (uint32_t)unhandle_int225, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(226, (uint32_t)unhandle_int226, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(227, (uint32_t)unhandle_int227, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(228, (uint32_t)unhandle_int228, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(229, (uint32_t)unhandle_int229, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(230, (uint32_t)unhandle_int230, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(231, (uint32_t)unhandle_int231, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(232, (uint32_t)unhandle_int232, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(233, (uint32_t)unhandle_int233, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(234, (uint32_t)unhandle_int234, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(235, (uint32_t)unhandle_int235, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(236, (uint32_t)unhandle_int236, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(237, (uint32_t)unhandle_int237, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(238, (uint32_t)unhandle_int238, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(239, (uint32_t)unhandle_int239, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(240, (uint32_t)unhandle_int240, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(241, (uint32_t)unhandle_int241, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(242, (uint32_t)unhandle_int242, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(243, (uint32_t)unhandle_int243, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(244, (uint32_t)unhandle_int244, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(245, (uint32_t)unhandle_int245, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(246, (uint32_t)unhandle_int246, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(247, (uint32_t)unhandle_int247, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(248, (uint32_t)unhandle_int248, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(249, (uint32_t)unhandle_int249, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(250, (uint32_t)unhandle_int250, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(251, (uint32_t)unhandle_int251, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(252, (uint32_t)unhandle_int252, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(253, (uint32_t)unhandle_int253, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(254, (uint32_t)unhandle_int254, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	idt_set_gate(255, (uint32_t)unhandle_int255, KERNEL_CS, IDT_INT_GATE_32 | IDT_PRESENT | IDT_DPL_0);
	//	dump_chun25k((uint32_t)&idt, 1);
	load_idt();
	bxbp();
}

// 测试中断0 除〇异常
extern void d_test_int0(void);

#endif