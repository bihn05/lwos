#include <lwos.h>
#include <video.h>
#include <string.h>
#include <pristdio.h>
#include <kb.h>
#include <pci.h>
#include <videocard.h>
#include <mem.h>
#include <assert.h>
#include <vsprintf.h>

int magic = LWOS_MAGIC;
char message[] = "ITS WORKING.\nkk";
PCI_HEADER pci_header;
PCI_HEADER pci_hd_display;
PCI_POS pci_display;
void kernel_init() {
	writereg_video(g_640x480x2);
	memset((char*)0xa0000, 0, 43200);
	outstr("### === !!! Wellcum to LWOS Kernel !!! === ###\n");

	for (int i = 0; i < 10; i++) {
		printk("Hello! %d\n", i);
	}

//	InitKB();

}

/*
FindPCI_GPU(&pci_display);
DGetDeviceInfo(&pci_hd_display, pci_display);
pci_info(&pci_hd_display);

uint32_t mmio_base = pci_hd_display.bar0;
volatile uint32_t* mmio = (volatile uint32_t*)mmio_base;

SetResolution(mmio, 1280, 800);
SetColorDepth(mmio, 32);

uint32_t fb_base = 0xe0000000;
uint32_t fb_size = 0x003e8000;
SetFrameBuffer(mmio, fb_base, fb_size);

volatile uint32_t* framebuffer = (volatile uint32_t*)fb_base;

putpixel(framebuffer,640,400,0x00ff00);*/