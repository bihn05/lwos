// kernel/main.c

#include <print.h>
#include <descript.h>
#include <interrupt.h>
#include <pic.h>
#include <printk.h>
#include <mm.h>
#include <disk.h>
#include <fsys/lwfs.h>
#include <task.h>
#include <tools.h>
#include <task/elf.h>

vfs_node_t* vfs_root = NULL;
video_t video;
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
    video.frame_buf = (uint8_t*)0xa0000;
    writereg_video(g_640x480x2);
    video_clear(&video);

    gdt_tss_init();
    idt_init();
    pmm_init();
    kheap_init();

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

    vfs_node_t* test_node = vfs_root->ops->finddir(vfs_root, "TEST.ELF");

    uint8_t* test_buf = (uint8_t*)kmalloc(512);
    int bytes_read = test_node->ops->read(test_node, 0, 512, test_buf);
    printk("Read %d bytes.\n", bytes_read);
    dump_chunk(test_buf, 1);
    uint64_t entry_point = load_elf(test_node);
    printk("ELF entry point: 0x%08X%08X\n", (uint32_t)(entry_point >> 32), (uint32_t)(entry_point & 0xFFFFFFFF));

    dump_chunk((void*)entry_point, 1);

    int (*entry_func)() = (int (*)())entry_point;
    uint32_t result = entry_func();
    printk("ELF returned: 0x%08X\n", result);

    while (1) {
        asm volatile ("hlt");
    }

    thread_a = thread_create("Task_A", 1, task_a, NULL);
    thread_b = thread_create("Task_B", 1, task_b, NULL);

    current_thread = thread_a;

    __asm__ volatile ("cli");
    init_multitasking();
    timer_init(100);

    __asm__ volatile ("sti");

}