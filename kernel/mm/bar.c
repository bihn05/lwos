#include <mm/bar.h>
#include <driver/pci.h>

bool pci_get_bar_info(pci_device_t *dev, int bar_index, pci_bar_info_t *out) {
    if (!out || bar_index < 0 || bar_index > 5) return false;

    uint8_t off = 0x10 + bar_index * 4;
    uint32_t bar = pci_config_read(dev->bus, dev->slot, dev->func, off);

    if (bar == 0 || bar == 0xFFFFFFFF) {
        out->type = PCI_BAR_NONE;
        return false;
    }

    if (bar & 0x1) {
        out->type = PCI_BAR_IO;
        out->base = (uint64_t)(bar & ~0x3U);
        out->prefetchable = false;
        out->size = 0;
        return true;
    }

    uint32_t mem_type = (bar >> 1) & 0x3;
    out->prefetchable = ((bar >> 3) & 0x1) != 0;

    if (mem_type == 0x0) {
        out->type = PCI_BAR_MEM32;
        out->base = (uint64_t)(bar & ~0xFULL);
        out->size = 0;
        return true;
    }

    if (mem_type == 0x2) {
        uint32_t bar_hi = pci_config_read(dev->bus, dev->slot, dev->func, off + 4);
        out->type = PCI_BAR_MEM64;
        out->base = ((uint64_t)bar_hi << 32) | (uint64_t)(bar & ~0xFULL);
        out->size = 0;
        return true;
    }

    return false;
}