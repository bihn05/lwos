#ifndef _PCI_H_
#define _PCI_H_

#include <stdint.h>
#include <driver/io.h>

uint32_t pci_read(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset) {
    // structure address
    uint32_t address = (1U << 32) |
                        ((uint32_t)bus << 16) |
                        ((uint32_t)device << 11) |
                        ((uint32_t)func << 8) |
                        (offset & 0xfc);
    outl(address, 0xcf8);
    return inl(0xcfc);
}

void check_device(uint8_t bus, uint8_t device);

uint16_t pci_get_vendor_id(uint8_t bus, uint8_t device, uint8_t func) {
    uint32_t data = pci_read(bus, device, func, 0);
    return (uint16_t)(data & 0xffff);
}
void pci_scan_bus() {
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t dev = 0; dev < 32; dev++) {
            uint16_t vendor = pci_get_vendor_id(bus, dev, 0);
            if (vendor != 0xffff) {
                check_device(bus, dev);
            }
        }
    }
}
void check_function(uint8_t bus, uint8_t device, uint8_t func) {
    uint32_t class_rev = pci_read(bus, device, func, 0x08);
    uint8_t class_code = (class_rev >> 24) & 0xff;
    uint8_t subclass = (class_rev >> 16) & 0xff;

    printk("Found PCI Device: Bus%d Dev%d Func%d Class:%x Sub:%x", bus, device, func, class_code, subclass);
}
void check_device(uint8_t bus, uint8_t device) {
    uint8_t func = 0;

    uint16_t vendor = pci_get_vendor_id(bus, device, func);
    if (vendor == 0xffff)return;

    check_function(bus, device, func);

    // read header type
    uint32_t header_type_reg = pci_read(bus, device, func, 0x0c);
    uint8_t header_type = (header_type_reg >> 16) & 0xff;

    if (header_type & 0x80) {
        for (func = 1; func < 8; func++) {
            if (pci_get_vendor_id(bus, device, func) != 0xffff) {
                check_function(bus, device, func);
            }
        }
    }
}
#endif