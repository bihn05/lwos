#include <driver/graphic/vbe.h>
#include <driver/pci.h>
#include <printk.h>
#include <mm.h>
#include <string.h>

#define BGA_INDEX_PORT 0x01CE
#define BGA_DATA_PORT  0x01CF

static inline void bga_write(uint16_t index, uint16_t value) {
    outw(index, BGA_INDEX_PORT);
    outw(value, BGA_DATA_PORT);
}

static inline uint16_t bga_read(uint16_t index) {
    outw(index, BGA_INDEX_PORT);
    return inw(BGA_DATA_PORT);
}

extern void *mmio_map_region(uint64_t pm4_pa, uint64_t phys_base, uint64_t size);

void vbe_write(uint16_t index, uint16_t value) {
    outw(index, VBE_DISPI_IOPORT_INDEX);
    outw(value, VBE_DISPI_IOPORT_DATA);
}

uint16_t vbe_read(uint16_t index) {
    outw(index, VBE_DISPI_IOPORT_INDEX);
    return inw(VBE_DISPI_IOPORT_DATA);
}

static bool bga_probe_regs(void) {
    uint16_t id = vbe_read(VBE_DISPI_INDEX_ID);
    printk("BGA ID = 0x%04x\n", id);
    return id >= VBE_DISPI_ID0 && id <= VBE_DISPI_ID5;
}

bool bga_detect_and_map(void) {
    pci_addr_t addr;
    if (!bga_find_pci_device(&addr)) {
        printk("BGA PCI device not found\n");
        return false;
    }

    g_bga.pci = addr;

    printk("BGA PCI @ %u:%u.%u\n", addr.bus, addr.slot, addr.func);

    pci_enable_device_mem(addr.bus, addr.slot, addr.func);

    pci_bar_info_t bar0 = pci_read_bar(addr.bus, addr.slot, addr.func, 0);
    printk("BGA BAR0: base=%p size=%p is_io=%d is_64=%d prefetch=%d\n",
           (void*)bar0.base, (void*)bar0.size,
           bar0.is_io, bar0.is_64, bar0.prefetchable);

    if (bar0.is_io || bar0.base == 0) {
        printk("BGA BAR0 is not a usable framebuffer memory BAR\n");
        return false;
    }

    g_bga.fb_phys = bar0.base;
    g_bga.fb_size = bar0.size;

    if (!bga_probe_regs()) {
        printk("BGA VBE regs not responding\n");
        return false;
    }

    return true;
}

bool bga_set_mode(uint64_t pm4_pa, uint32_t width, uint32_t height, uint32_t bpp) {
    if (g_bga.fb_phys == 0) {
        printk("BGA not initialized\n");
        return false;
    }

    vbe_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
    vbe_write(VBE_DISPI_INDEX_XRES, (uint16_t)width);
    vbe_write(VBE_DISPI_INDEX_YRES, (uint16_t)height);
    vbe_write(VBE_DISPI_INDEX_BPP,  (uint16_t)bpp);
    vbe_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED);

    g_bga.width  = width;
    g_bga.height = height;
    g_bga.bpp    = bpp;
    g_bga.pitch  = width * (bpp / 8);

    uint64_t need = (uint64_t)g_bga.pitch * g_bga.height;

    if (g_bga.fb_size && need > g_bga.fb_size) {
        printk("BGA mode requires %p bytes but BAR has %p bytes\n",
               (void*)need, (void*)g_bga.fb_size);
        return false;
    }

    g_bga.fb_virt = mmio_map_region(pm4_pa, g_bga.fb_phys, need);
    if (!g_bga.fb_virt) {
        printk("BGA framebuffer map failed\n");
        return false;
    }

    printk("BGA mode %ux%ux%u fb_phys=%p fb_virt=%p pitch=%u\n",
           g_bga.width, g_bga.height, g_bga.bpp,
           (void*)g_bga.fb_phys, g_bga.fb_virt, g_bga.pitch);

    return true;
}

void bga_fill(uint32_t color) {
    if (!g_bga.fb_virt || g_bga.bpp != 32) return;

    volatile uint32_t *fb = (volatile uint32_t*)g_bga.fb_virt;
    uint64_t pixels = (uint64_t)g_bga.width * g_bga.height;

    for (uint64_t i = 0; i < pixels; i++) {
        fb[i] = color;
    }
}