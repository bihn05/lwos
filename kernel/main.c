#include <lwos.h>
#include <type.h>
#include <video.h>
#include <vsprintf.h>
#include <gd.h>
#include <memory.h>
#include <interrupt.h>

int magic = LWOS_MAGIC;
void kernel_init() {
	writereg_video(g_640x480x16);
	cleardevice();

	ColorAttr = 0x1f;
	for (int i = 0; i < 4800; i++)outstr(" ");
	CursorX = CursorY = 0;
	printk("Welcome to LWOS Kernel!!!\n");

	InitGDT();

//	printk("Available Memory = %d KB\n", total_mem() >> 10);

	interrupt_init();

	memory_init();
	memory_map_init();
	mapping_init();
}
