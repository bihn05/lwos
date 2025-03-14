#ifndef _VIDEO_CARD_H_
#define _VIDEO_CARD_H_

#include <pristdio.h>
#include <port.h>
#include <pci.h>

void FindGPUDev(uint8_t * bus, uint8_t * dev, uint8_t * func) {
	for (uint8_t b=0;b<256;b++) {
		for (uint8_t d=0;d<32;d++) {
			for (uint8_t f=0;f<8;f++) {
				uint32_t class_code = pci_read(b,d,f,0x08) >> 8;
				if (class_code == 0x03000 || class_code == 0x038000) {
					*bus = b;
					*dev = d;
					*func = f;
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
#endif
