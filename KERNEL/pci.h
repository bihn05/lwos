#ifndef _PCI_H_
#define _PCI_H_

#include <stdint.h>
#include <pristdio.h>
#include <port.h>

uint32_t pci_read(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset) {
	uint32_t address = 0x80000000|(bus<<16)|(dev<<11)|(func<<8)|(offset&0xfc);
	outl(address, 0xcf8);
	return inl(0xcfc);
}
void pci_write(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint32_t value) {
	uint32_t address = 0x80000000|(bus<<16)|(dev<<11)|(func<<8)|(offset&0xfc);
	outl(address, 0xcf8);
	outl(value, 0xcfc);
}
void GetDeviceInfo(uint32_t* dist, uint8_t bus, uint8_t dev, uint8_t func) {
	
}
uint32_t GetMMIOBase(uint8_t bus, uint8_t dev, uint8_t func) {
	uint32_t base = pci_read(bus, dev, func, 0x10);
	return base & 0xf0;
}

#endif
