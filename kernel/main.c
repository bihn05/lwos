#include <kernel.h>
#include <interrupt.h>
#include <descript.h>
//#include <mem.h>
#include <driver/ata.h>
#include <driver/kbc.h>
#include <timer.h>
#include <mem.h>
#include <string.h>

#include <conio.h>
void display_banner() {
	printk("%s\n", " ___       ___       __   ________  ________      ");
	printk("%s\n", "|\\  \\     |\\  \\     |\\  \\|\\   __  \\|\\   ____\\     ");
	printk("%s\n", "\\ \\  \\    \\ \\  \\    \\ \\  \\ \\  \\|\\  \\ \\  \\___|_    ");
	printk("%s\n", " \\ \\  \\    \\ \\  \\  __\\ \\  \\ \\  \\\\\\  \\ \\_____  \\   ");
	printk("%s\n", "  \\ \\  \\____\\ \\  \\|\\__\\_\\  \\ \\  \\\\\\  \\|____|\\  \\  ");
	printk("%s\n", "   \\ \\_______\\ \\____________\\ \\_______\\____\\_\\  \\ ");
	printk("%s\n", "    \\|_______|\\|____________|\\|_______|\\_________\\");
	printk("%s\n", "                                      \\|_________|");
	printk("%s\n", "                                                  ");
	printk("%s\n", "   CODE is COSTARICA                              ");
}
static void fa(void) {
	printk("A-");
	send_eoi(0);
}
void kernel_init() {
	writereg_video(g_640x480x2);
	clear_device();
	calc_mem();
	display_banner();
	while (1);
	printk(" i Init GDT\n");
	gdt_init();
	printk(" i Init Interrupts\n");
	idt_init();
	__asm volatile("sti");
	d_test_int0();
	printk(" i Init Timer");
	init_pit();
	while (1);
}