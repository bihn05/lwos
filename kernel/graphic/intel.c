#include <driver/graphic/intel.h>

bool intel_gpu_probe_0x2A42(pci_display_device_t *disp, intel_gpu_info_t *out) {
    if (!disp || !out) return false;
    if (disp->vendor_id != 0x8086) return false;

    memset(out, 0, sizeof(*out));

    out->addr = disp->addr;
    out->vendor_id = disp->vendor_id;
    out->device_id = disp->device_id;

    printk("[video] Intel GPU detected: vendor_id=0x%04X device_id=0x%04X\n",
           out->vendor_id, out->device_id);

    // 先把所有相关 BAR 打出来
    for (int i = 0; i < 4; i++) {
        pci_bar_info_t bar = pci_read_bar(disp->addr.bus, disp->addr.slot, disp->addr.func, i);
        printk("[video] BAR%d: base=0x%08X%08X size=0x%08X%08X type=%04X\n",
               i,
               (uint32_t)(bar.base >> 32), (uint32_t)(bar.base & 0xFFFFFFFF),
               (uint32_t)(bar.size >> 32), (uint32_t)(bar.size & 0xFFFFFFFF),
               bar.type);
    }
    
    uint32_t cmd = pci_config_read(disp->addr.bus, disp->addr.slot, disp->addr.func, 0x04);
    printk("[video] PCI CMD/STS = 0x%08X\n", cmd);

    pci_bar_info_t bar0 = pci_read_bar(disp->addr.bus, disp->addr.slot, disp->addr.func, 0);
    pci_bar_info_t bar2 = pci_read_bar(disp->addr.bus, disp->addr.slot, disp->addr.func, 2);

    // Intel 集显通常 BAR0 = GTT/MMIO，BAR2 = aperture
    if (bar0.type == PCI_BAR_MEM32 || bar0.type == PCI_BAR_MEM64) {
        out->mmio_bar = bar0;
    }

    if (bar2.type == PCI_BAR_MEM32 || bar2.type == PCI_BAR_MEM64) {
        out->aper_bar = bar2;
    }

    return true;
}