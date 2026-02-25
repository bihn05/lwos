// kernel/main.c

#include <print.h>
#include <descript.h>
#include <interrupt.h>
#include <pic.h>
#include <printk.h>
#include <mm.h>
#include <disk.h>

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
    timer_init(100);

    display_banner();

    pmm_init();
    kheap_init();

    ata_init();
    ata_detect_drives();

    uint64_t* test_ptr = (uint64_t*)kmalloc(1024);
    test_ptr[0] = 0;

    //__asm__ volatile ("sti");

    while (1) {
        asm volatile ("hlt");
    }
}