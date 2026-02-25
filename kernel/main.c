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
task_struct_t* thread_a = NULL;
task_struct_t* thread_b = NULL;
void kernel_init() {
    video.frame_buf = (uint8_t*)0xa0000;
    writereg_video(g_640x480x2);
    video_clear(&video);

    gdt_tss_init();
    idt_init();
    timer_init(100);

    display_banner();

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

    // 2. 组成环形链表 (Round-Robin 队列)
    thread_a = thread_create("Task_A", 1, task_a, NULL);
    thread_b = thread_create("Task_B", 1, task_b, NULL);

    // 3. 设定当前运行的线程
    current_thread = thread_a;

    while (1);

    __asm__ volatile ("sti");

    while (1) {
        asm volatile ("hlt");
    }
}