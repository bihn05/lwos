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
#include <driver/pci.h>
#include <network.h>
#include <mm/proc.h>
#include <graphics.h>
#include <driver/kbc.h>
#include <driver/tty.h>
#include <driver/video_probe.h>

vfs_node_t* vfs_root = NULL;
video_t video;
pci_addr_t g_bga_pci_addr;
uint64_t kernel_pm4;
uint64_t user_pm4;
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
    kernel_pm4 = get_cr3();
    user_pm4 = create_user_address_space(kernel_pm4);
    map_video_buffer(user_pm4);

    printk("Kernel PML4 at PA 0x%08X%08X\n", (uint32_t)(kernel_pm4 >> 32), (uint32_t)(kernel_pm4 & 0xFFFFFFFF));
    printk("User PML4 at PA 0x%08X%08X\n", (uint32_t)(user_pm4 >> 32), (uint32_t)(user_pm4 & 0xFFFFFFFF));

    if (!map_user_stack(user_pm4, USER_STACK_TOP, USER_STACK_SIZE)) {
        printk("Failed to map user stack\n");
        while (1) asm volatile ("hlt");
    }
    tss_late_init(user_pm4);
    ist_init(user_pm4);

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
    /*
    int create_result = vfs_root->ops->create(vfs_root, "MODIFY.TXT", VFS_FLAG_FILE);
    vfs_node_t* modify_test = vfs_root->ops->finddir(vfs_root, "MODIFY.TXT");
    const char* content = "Hello, LWOS!";
    modify_test->ops->write(modify_test, 0, strlen(content), (uint8_t*)content);
    memset(test_buf, 0, 512);
    modify_test->ops->read(modify_test, 0, 512, test_buf);
    printk("Content of MODIFY.TXT: %s\n", test_buf); */

    pci_check_all_buses();
    bga_detect_and_map();

    kbc_init();
    asm volatile ("sti");

    video_device_t g_video_dev;

    if (video_probe_primary(&g_video_dev)) {
        printk("Primary video device ready: %ux%ux%u\n",
               g_video_dev.width,
               g_video_dev.height,
               g_video_dev.bpp);
    } else {
        printk("No usable primary video device.\n");
        while (1) asm volatile ("hlt");
    }

    g_video_dev.ops->fill(&g_video_dev, 0x00ff00); // 黑屏

        while (1) asm volatile ("hlt");
        
    video_device_t g_dev;
    gfx_surface_t g_screen;
    tty_t tty;
    bochs_vbe_init(&g_dev);
    gfx_surface_from_device(&g_screen, &g_dev);
    tty_init(&tty, &g_screen, 0x00FF00, 0x000000, 8, 16);

    tty_write(&tty, "Welcome to LWOS!\0");

    while (1);

    vfs_node_t* test_node = vfs_root->ops->finddir(vfs_root, "TEST.ELF");

    uint64_t entry_point = load_elf(user_pm4, test_node);
    printk("ELF entry point: 0x%08X%08X\n", (uint32_t)(entry_point >> 32), (uint32_t)(entry_point & 0xFFFFFFFF));
    printk("kernel tss rsp0 = %p\n", kernel_tss.rsp0);

    video.frame_buf = (uint8_t*)0xFFFFFFFFC3000000ULL;
    pt_debug_walk(user_pm4, 0xFFFFFFFFC3000000ULL);

    // 切到目标进程地址空间
    asm volatile("mov %0, %%cr3" : : "r"(user_pm4) : "memory");

    __asm__ volatile (
        "mov $0x23, %%ax \n"
        "mov %%ax, %%ds \n"
        "mov %%ax, %%es \n"
        "mov %%ax, %%fs \n"
        "mov %%ax, %%gs \n"

        "pushq $0x23 \n"
        "pushq %0 \n"
        "pushq $0x202 \n"
        "pushq $0x1b \n"
        "pushq %1 \n"
        "iretq \n"
        :
        : "r"(USER_STACK_TOP), "r"(entry_point)
        : "memory", "ax"
    ); 

    while (1) {
        asm volatile ("hlt");
    }
/*
    current_thread = thread_a;
    thread_a = thread_create(user_pm4, "Task_A", 1, task_a, NULL);
    thread_b = thread_create(user_pm4, "Task_B", 1, task_b, NULL);

    init_multitasking();
    timer_init(100);
    __asm__ volatile ("sti");*/

    while (1) {
        asm volatile ("hlt");
    }

    /*
    pci_check_all_buses();
    pci_device_t network_dev;
    rtl8168_t my_nic;
    /*
    if (pci_find_device_by_class(0x02, 0x00, &network_dev)) {
        printk("NIC Found at %02x:%02x.%d | Vendor: %04x, Device: %04x\n", 
               network_dev.bus, network_dev.slot, network_dev.func, 
               network_dev.vendor_id, network_dev.device_id);
        rtl8168_init_and_read_mac(&network_dev, &my_nic);
        
    } else {
        printk("No Ethernet Controller found!\n");
    }*/

    /*

    pci_find_and_inspect_bridge_for_bus(8);

    if (!pci_find_device_by_class(0x02, 0x00, &network_dev))
        while (1) asm volatile ("hlt");

    printk("NIC Found at %02x:%02x.%d | Vendor: %04x, Device: %04x\n", 
    network_dev.bus, network_dev.slot, network_dev.func, 
    network_dev.vendor_id, network_dev.device_id);
    rtl8168_init_and_read_mac(&network_dev, &my_nic);
    printk("Entering network polling loop. Waiting for packets...\n");

    while (1) {
        rtl8168_poll_rx(&my_nic);
    
    // 如果你有键盘驱动，可以加个按键退出逻辑
    // if (kb_hit()) break; 
    }*/

}