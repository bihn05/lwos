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
void demo_fs(vfs_node_t* root) {
	// find file
	// vfs_node_t* file = root->ops->finddir(root, "TEST.TXT");
	vfs_node_t* file = lwfs_resolve_path((lwfs_instance_t*)root->fs_instance, "/TEST.TXT");
	
	if (file) {
		printk("Found file: %s, size: %d bytes, start cluster: %d\n", 
			file->name, file->size, (uint32_t)file->fs_private_data);
		
		uint8_t* buffer = (uint8_t*)kmalloc(512);
		int bytes_read = file->ops->read(file, 0, 512, buffer);
		if (bytes_read > 0) {
			printk("Read %d bytes from file:\n", bytes_read);
			dump_chunk(buffer, 1);
			printk("\n");
		} else {
			printk("Failed to read from file.\n");
		}
		kfree(buffer);
	} else {
		printk("File not found in root directory.\n");
	}
}
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

	demo_fs(vfs_root);

	while (1) {
		__asm volatile("hlt");
	}

	init_multitasking();

	create_kernel_thread(task_a);
	create_kernel_thread(task_b);

	pit_init();
	//cheax(); // eax = 0xcafebabe

	__asm volatile("sti"); // enable interrupts

	bxbp();
	cheax();
	while (1);
}