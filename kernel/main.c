// kernel/main.c

#include <kernel.h>
#include <interrupt.h>
#include <descript.h>
#include <driver/ata.h>
#include <driver/kbc.h>
#include <driver/acpi.h>
#include <timer.h>
#include <mem.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <mm/pcb.h>
#include <driver/pci.h>
#include <string.h>
#include <debug.h>
#include <speaker.h>
#include <driver/graphics.h>
#include <fsys/lwfs.h>
#include <fpu.h>

#include <sdbg/superdebug.h>

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
	printk("   CODE is COSTARICA                              \n\n");
}
void kernel_init() {
	writereg_video(g_640x480x2);
	clear_device();
	printk(" i Init GDT\n");
	gdt_init();
	printk(" i Init Interrupts\n");
	idt_init();
	vpic_init();
	keyboard_init();
	IRQ_set_mask(0);
	if (!check_acpi_support()) {
		printk(" * ACPI not supported.\n");
	}
//	enable_spk();
	pmm_init();
	vmm_init();
	kheap_init();
	display_banner();
	pci_scan_bus();
	init_fpu();

	ata_init();
	ata_detect_drives();

	init_multitasking();

	create_kernel_thread(task_a);
	create_kernel_thread(task_b);

	pit_init();
	//cheax(); // eax = 0xcafebabe

	__asm volatile("sti"); // enable interrupts

	while (1) {
		__asm volatile("hlt");
	}

	super_dbg();

	bxbp();
	cheax();
	while (1);
}