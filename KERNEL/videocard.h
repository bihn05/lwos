#ifndef _VIDEO_CARD_H_
#define _VIDEO_CARD_H_

#include <pristdio.h>
#include <port.h>
#include <pci.h>

void FindPCI_GPU(PCI_POS* pos) {
	PCI_HEADER buf;
	PCI_POS p;
	for (int b = 0; b < 256; b++) {
		for (int d = 0; d < 32; d++) {
			for (int f = 0; f < 8; f++) {
				p.bus = b;
				p.dev = d;
				p.func = f;
				GetDeviceInfo(&buf, p);
				if (buf.class_code == 0x03) {
					outstr("Found Display\n");
					*pos = p;
					return;
				}
			}
		}
	}
}
void SetResolution(volatile uint32_t* mmio, uint16_t width, uint16_t height) {
	mmio[0x10] = width;
	mmio[0x12] = height;
}
void SetColorDepth(volatile uint32_t* mmio, uint8_t bpp) {
	mmio[0x14] = bpp;
}
void SetFrameBuffer(volatile uint32_t* mmio, uint32_t base, uint32_t size) {
	mmio[0x20] = base;  // 设置帧缓冲区基地址
	mmio[0x24] = size;  // 设置帧缓冲区大小
}
void putpixel(volatile uint32_t* framebuffer, uint16_t x, uint16_t y, uint32_t color) {
	framebuffer[y * 1280 + x] = color; // 1280为水平分辨率
}
#endif
