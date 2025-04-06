#include <lwos.h>
#include <stdint.h>
#include <video.h>
#include <string.h>
#include <pristdio.h>
#include <kb.h>
//#include <pci.h>
#include <videocard.h>
#include <mem.h>
#include <assert.h>
#include <vsprintf.h>
#include <global.h>
#include <task.h>

int magic = LWOS_MAGIC;
char message[] = "ITS WORKING.\nkk";
PCI_HEADER pci_header;
PCI_HEADER pci_hd_display;
PCI_POS pci_display;
void kernel_init() {
	writereg_video(g_640x480x16);
	cleardevice();

	ColorAttr = 0x1f;
	for (int i = 0; i < 4800; i++)outstr(" ");
	CursorX = CursorY = 0;
	printk("Welcome to LWOS Kernel!!!\n");
	InitGDT();
	task_init();
}
