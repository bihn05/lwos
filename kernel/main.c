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

    /*
    uint8_t* test_buf = (uint8_t*)kmalloc(512);
    int bytes_read = test_node->ops->read(test_node, 0, 512, test_buf);
    printk("Read %d bytes.\n", bytes_read);
    dump_chunk(test_buf, 1);

    int create_result = vfs_root->ops->create(vfs_root, "MODIFY.TXT", VFS_FLAG_FILE);
    vfs_node_t* modify_test = vfs_root->ops->finddir(vfs_root, "MODIFY.TXT");
    const char* content = "Hello, LWOS!";
    modify_test->ops->write(modify_test, 0, strlen(content), (uint8_t*)content);
    memset(test_buf, 0, 512);
    modify_test->ops->read(modify_test, 0, 512, test_buf);
    printk("Content of MODIFY.TXT: %s\n", test_buf); 

    uint64_t entry_point = load_elf(test_node);
    printk("ELF entry point: 0x%08X%08X\n", (uint32_t)(entry_point >> 32), (uint32_t)(entry_point & 0xFFFFFFFF));

    dump_chunk((void*)entry_point, 1);

    int (*entry_func)() = (int (*)())entry_point;
    uint32_t result = entry_func();
    printk("ELF returned: 0x%08X\n", result); */

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
    }

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