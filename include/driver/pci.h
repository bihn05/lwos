#ifndef _PCI_H
#define _PCI_H

#include <stdint.h>
#include <driver/port.h>

#define BGA_PCI_VENDOR_ID 0x1234
#define BGA_PCI_DEVICE_ID 0x1111

#define VBE_DISPI_IOPORT_INDEX 0x01CE
#define VBE_DISPI_IOPORT_DATA  0x01CF

#define VBE_DISPI_INDEX_ID         0x0
#define VBE_DISPI_INDEX_XRES       0x1
#define VBE_DISPI_INDEX_YRES       0x2
#define VBE_DISPI_INDEX_BPP        0x3
#define VBE_DISPI_INDEX_ENABLE     0x4
#define VBE_DISPI_INDEX_BANK       0x5
#define VBE_DISPI_INDEX_VIRT_WIDTH 0x6
#define VBE_DISPI_INDEX_VIRT_HEIGHT 0x7
#define VBE_DISPI_INDEX_X_OFFSET   0x8
#define VBE_DISPI_INDEX_Y_OFFSET   0x9

#define VBE_DISPI_DISABLED       0x00
#define VBE_DISPI_ENABLED        0x01
#define VBE_DISPI_GETCAPS        0x02
#define VBE_DISPI_8BIT_DAC       0x20
#define VBE_DISPI_LFB_ENABLED    0x40
#define VBE_DISPI_NOCLEARMEM     0x80

#define VBE_DISPI_ID0 0xB0C0
#define VBE_DISPI_ID1 0xB0C1
#define VBE_DISPI_ID2 0xB0C2
#define VBE_DISPI_ID3 0xB0C3
#define VBE_DISPI_ID4 0xB0C4
#define VBE_DISPI_ID5 0xB0C5

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

typedef struct pci_addr {
    uint8_t bus, slot, func;
} pci_addr_t;

typedef struct bga_device {
    pci_addr_t pci;

    uint64_t fb_phys;
    size_t   fb_size;
    void    *fb_virt;

    uint32_t width;
    uint32_t height;
    uint32_t bpp;
    uint32_t pitch;

    bool     has_mmio_bar2;
    uint64_t mmio_phys;
    size_t   mmio_size;
    volatile uint8_t *mmio_virt;
} bga_device_t;

static bga_device_t g_bga;

typedef enum {
    PCI_BAR_NONE = 0,
    PCI_BAR_IO,
    PCI_BAR_MEM32,
    PCI_BAR_MEM64
} pci_bar_type_t;

typedef struct pci_bar_info {
    uint64_t base;
    uint64_t size;
    pci_bar_type_t type;
    bool     is_io;
    bool     is_64;
    bool     prefetchable;
} pci_bar_info_t;

// search for a PCI device by class code and subclass, return true if found and fill out_dev
bool pci_find_device_by_class(uint8_t target_class, uint8_t target_subclass, pci_device_t *out_dev);
pci_bar_info_t pci_read_bar(uint8_t bus, uint8_t slot, uint8_t func, uint8_t bar_index);
void pci_enable_device_mem(uint8_t bus, uint8_t slot, uint8_t func);
bool bga_find_pci_device(pci_addr_t *out);
uint16_t pci_get_vendor_id(uint8_t bus, uint8_t slot, uint8_t func);
uint16_t pci_get_device_id(uint8_t bus, uint8_t slot, uint8_t func);
uint16_t pci_addr_vendor_id(pci_addr_t addr);
uint16_t pci_addr_device_id(pci_addr_t addr);

typedef struct {
    bool found;
    pci_addr_t addr;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t class_code;
    uint8_t subclass;
    uint8_t prog_if;
} pci_display_device_t;

#endif