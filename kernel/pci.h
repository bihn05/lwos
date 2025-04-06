#ifndef _PCI_H_
#define _PCI_H_

#include <stdint.h>
#include <pristdio.h>
#include <port.h>

typedef struct PCI_POS {
	uint8_t bus;
	uint8_t dev;
	uint8_t func;
} PCI_POS;
typedef struct PCI_HEADER {
	uint16_t vendor_id;
	uint16_t device_id;
	uint16_t command;
	uint16_t status;
	uint8_t revision_id;
	uint8_t program_interface;
	uint8_t subclass;
	uint8_t class_code;
	uint8_t cache_line_size;
	uint8_t latency_timer;
	uint8_t header_type;
	uint8_t bist;
	uint32_t bar0;
	uint32_t bar1;
	uint8_t buffer[232];
} PCI_HEADER ;

uint32_t pci_read(PCI_POS pos, uint8_t offset) {
	uint32_t address = 0x80000000 | (pos.bus << 16) | (pos.dev << 11) | (pos.func << 8) | (offset & 0xfc);
	outl(address, 0xcf8);
	return inl(0xcfc);
}
void pci_write(PCI_POS pos, uint32_t value, uint8_t offset) {
	uint32_t address = 0x80000000 | (pos.bus << 16) | (pos.dev << 11) | (pos.func << 8) | (offset & 0xfc);
	outl(address, 0xcf8);
	outl(value, 0xcfc);
}
void DGetDeviceInfo(uint32_t* dist, PCI_POS pos) {
	outstr("bus=");
	iouthex8(pos.bus);
	outstr(" dev=");
	iouthex8(pos.dev);
	outstr(" func=");
	iouthex8(pos.func);
	outstr(" ");
	for (int i = 0; i < 64; i++) {
		*dist = pci_read(pos, i * 4);
		dist++;
	}
}
void GetDeviceInfo(uint32_t* dist, PCI_POS pos) {
	for (int i = 0; i < 64; i++) {
		*dist = pci_read(pos, i * 4);
		dist++;
	}
}
void pci_info(PCI_HEADER* pci) {
	outstr("Vendor ID = 0x");
	iouthex16(pci->vendor_id);
	outstr(", Device ID = 0x");
	iouthex16(pci->device_id);
	outstr(", Class = 0x");
	iouthex8(pci->class_code);
	iouthex8(pci->subclass);
	outstr("\n");
}

#endif
