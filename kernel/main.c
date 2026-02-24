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
#include <math/math.h>
#include <fsys/elf.h>

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
vfs_node_t* vfs_root = NULL;

typedef struct {
	double r;
	double a, w;
} ball_t;
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

	if (ata_get_device_count() > 0) {
		block_dev_t* sda = ata_get_block_device_ptr(0);

		if (sda != NULL) {
			vfs_root = lwfs_mount(sda);

			if (vfs_root != NULL) {
				printk("LWFS mounted successfully on %s\n", sda->dev_name);
			} else {
				printk("Failed to mount LWFS on %s\n", sda->dev_name);
			}
		}
	} else {
		printk("No ATA devices found, skipping LWFS mount.\n");
	}

	vfs_node_t* test_exec = lwfs_resolve_path((lwfs_instance_t*)vfs_root->fs_instance, "/TEST.ELF");
	uint32_t entry_point = load_elf(test_exec);
	create_user_process(entry_point);

	dump_chunk((uint8_t*)0x01fff000, 1);

	pit_init();

	__asm volatile("sti"); // enable interrupts

	while (1) {
		__asm volatile("hlt");
	}

	bxbp();
	cheax();
	while (1);
}