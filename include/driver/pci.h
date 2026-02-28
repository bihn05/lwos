#ifndef _PCI_H
#define _PCI_H

#include <stdint.h>
#include <driver/port.h>

uint32_t pci_config_read(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
uint16_t pci_config_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
void pci_config_write(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value);
void pci_config_write_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t value);
void pci_check_all_buses(void);
void pci_check_device(uint8_t bus, uint8_t slot);
void pci_check_function(uint8_t bus, uint8_t slot, uint8_t func);
uint64_t pci_get_bar_size(uint8_t b, uint8_t s, uint8_t f, uint8_t bar_idx);

typedef struct {
    uint8_t  bus;
    uint8_t  slot;       // slot
    uint8_t  func;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t  class_code;
    uint8_t  subclass;
} pci_device_t;

// search for a PCI device by class code and subclass, return true if found and fill out_dev
bool pci_find_device_by_class(uint8_t target_class, uint8_t target_subclass, pci_device_t *out_dev);

#endif