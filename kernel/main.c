#include <kernel.h>
#include <interrupt.h>
#include <descript.h>
#include <driver/ata.h>
#include <driver/kbc.h>
#include <timer.h>
#include <mem.h>
#include <string.h>
#include <debug.h>
#include <speaker.h>
void display_banner() {
	printk(" ___       ___       __   ________  ________      \n");
	printk("|\\  \\     |\\  \\     |\\  \\|\\   __  \\|\\   ____\\     \n");
	printk("\\ \\  \\    \\ \\  \\    \\ \\  \\ \\  \\|\\  \\ \\  \\___|_    \n");
	printk(" \\ \\  \\    \\ \\  \\  __\\ \\  \\ \\  \\\\\\  \\ \\_____  \\   \n");
	printk("  \\ \\  \\____\\ \\  \\|\\__\\_\\  \\ \\  \\\\\\  \\|____|\\  \\  \n");
	printk("   \\ \\_______\\ \\____________\\ \\_______\\____\\_\\  \\ \n");
	printk("    \\|_______|\\|____________|\\|_______|\\_________\\\n");
	printk("                                      \\|_________|\n");
	printk("                                                  \n");
	printk("   CODE is COSTARICA                              \n");
}
void kernel_init() {
	writereg_video(g_640x480x2);
	clear_device();
	display_banner();
	calc_mem();
	printk(" i Init GDT\n");
	gdt_init();
	printk(" i Init Interrupts\n");
	idt_init();
	vpic_init();
	keyboard_init();
	pit_init();
	IRQ_set_mask(0);
	printk("miao");
	__asm("sti");
	set_spk_freq(440);
	enable_spk();
	bxbp();
	cheax();
	while (1);
}