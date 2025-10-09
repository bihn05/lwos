#include <kernel.h>
#include <interrupt.h>
#include <descript.h>
#include <driver/ata.h>
#include <driver/kbc.h>
#include <driver/acpi.h>
#include <timer.h>
#include <mem.h>
#include <string.h>
#include <debug.h>
#include <speaker.h>
#include <driver/graphics.h>
#include <fsys/lwfs.h>
#include <mm/vmm.h>

#include <conio.h>
#include <sdbg/superdebug.h>

char testy[256];


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
	printk(" i Init GDT\n");
	gdt_init();
	printk(" i Init Interrupts\n");
	idt_init();
	mmap_init();
	vpic_init();
	keyboard_init();
//	pit_init();
	IRQ_set_mask(0);
	__asm("sti");
	if (!check_acpi_support()) {
		printk(" * ACPI not supported.\n");
	}
//	enable_spk();
	ata_init();
	ata_detect_drives();
	load_mbr();
	fat_t tet;
	get_fat_entry(&tet, 0);
	fat_info(&tet);

	super_dbg();

	bxbp();
	cheax();
	while (1);
}