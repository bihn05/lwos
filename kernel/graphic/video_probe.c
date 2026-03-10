#include <driver/video_probe.h>
#include <stdint.h>
#include <string.h>

#include <driver/graphic/intel.h>

#define VBE_DISPI_IOPORT_INDEX 0x01CE
#define VBE_DISPI_IOPORT_DATA  0x01CF
#define VBE_DISPI_INDEX_ID     0x0

#define VBE_DISPI_ID0 0xB0C0
#define VBE_DISPI_ID1 0xB0C1
#define VBE_DISPI_ID2 0xB0C2
#define VBE_DISPI_ID3 0xB0C3
#define VBE_DISPI_ID4 0xB0C4
#define VBE_DISPI_ID5 0xB0C5

static bool bochs_vbe_present(void) {
    uint16_t id;

    // 如果你的 outw(value, port) 是历史写法，就保留你自己的顺序
    outw(VBE_DISPI_INDEX_ID, VBE_DISPI_IOPORT_INDEX);
    id = inw(VBE_DISPI_IOPORT_DATA);

    printk("Bochs VBE ID = 0x%04X\n", id);

    return (id >= VBE_DISPI_ID0 && id <= VBE_DISPI_ID5);
}

bool pci_find_display_device(pci_display_device_t *out) {
    if (!out) return false;

    memset(out, 0, sizeof(*out));

    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {

            // func 0 先读，判断设备是否存在
            uint32_t id0 = pci_config_read(bus, slot, 0, 0x00);
            if (id0 == 0xFFFFFFFF) {
                continue;
            }

            // 判断是不是 multifunction
            uint32_t hdr = pci_config_read(bus, slot, 0, 0x0C);
            uint8_t header_type = (hdr >> 16) & 0xFF;
            uint8_t func_count = (header_type & 0x80) ? 8 : 1;

            for (uint8_t func = 0; func < func_count; func++) {
                uint32_t id = pci_config_read(bus, slot, func, 0x00);
                if (id == 0xFFFFFFFF) {
                    continue;
                }

                uint16_t vendor = (uint16_t)(id & 0xFFFF);
                uint16_t device = (uint16_t)((id >> 16) & 0xFFFF);

                uint32_t class_reg = pci_config_read(bus, slot, func, 0x08);
                uint8_t prog_if    = (class_reg >> 8)  & 0xFF;
                uint8_t subclass   = (class_reg >> 16) & 0xFF;
                uint8_t class_code = (class_reg >> 24) & 0xFF;

                if (class_code == 0x03) {
                    out->found = true;
                    out->addr.bus = bus;
                    out->addr.slot = slot;
                    out->addr.func = func;
                    out->vendor_id = vendor;
                    out->device_id = device;
                    out->class_code = class_code;
                    out->subclass = subclass;
                    out->prog_if = prog_if;
                    return true;
                }
            }
        }
    }

    return false;
}

bool video_probe_primary(video_device_t *dev) {
    if (!dev) return false;

    memset(dev, 0, sizeof(*dev));

    // --------------------------------------------------
    // 1. 优先探测 Bochs VBE
    // --------------------------------------------------
    if (bochs_vbe_present()) {
        printk("[video] Bochs VBE detected, trying bochs_vbe_init...\n");
        if (bochs_vbe_init(dev)) {
            printk("[video] Bochs VBE attached successfully.\n");
            return true;
        }
        printk("[video] Bochs VBE present but init failed.\n");
    }

    // --------------------------------------------------
    // 2. 扫描 PCI display controller
    // --------------------------------------------------
    pci_display_device_t disp;
    if (!pci_find_display_device(&disp)) {
        printk("[video] No PCI display controller found.\n");
        return false;
    }

    printk("[video] PCI display controller found at %02X:%02X.%u "
           "vendor=%04X device=%04X class=%02X subclass=%02X prog_if=%02X\n",
           disp.addr.bus, disp.addr.slot, disp.addr.func,
           disp.vendor_id, disp.device_id,
           disp.class_code, disp.subclass, disp.prog_if);

    // 当前先只识别，不初始化实体机显卡
    // 以后你可以在这里按 vendor/device 绑定不同驱动：
    //
    if (disp.vendor_id == 0x8086) return intel_gpu_init(dev, &disp);
    // if (disp.vendor_id == 0x10DE) return nvidia_init(dev, &disp);
    // if (disp.vendor_id == 0x1002) return amd_init(dev, &disp);
    //
    printk("[video] No native driver bound for this display controller yet.\n");
    return false;
}
extern uint64_t kernel_pm4;
bool intel_gpu_init(video_device_t *dev, pci_display_device_t *disp) {
    if (!dev || !disp) return false;

    intel_gpu_info_t info;
    if (!intel_gpu_probe(disp, &info)) {
        return false;
    }

    info.mmio_virt = mmio_map_region(kernel_pm4, info.mmio_bar.base, info.mmio_bar.size, PTE_PCD | PTE_PWT);
    uint32_t v0 = mmio_read32(info.mmio_virt, 0x0000);
    uint32_t v1 = mmio_read32(info.mmio_virt, 0x0004);
    printk("intel mmio test: 0x%08X 0x%08X\n", v0, v1);

    printk("[video] Intel GPU detected: vendor_id=0x%04X device_id=0x%04X\n",
           info.vendor_id, info.device_id);

    if (info.mmio_bar.type != PCI_BAR_MEM32 &&
        info.mmio_bar.type != PCI_BAR_MEM64) {
        printk("[video] Intel GPU present, but MMIO BAR0 is not usable yet.\n");
        return false;
    }

    // 先只做 MMIO 映射尝试
    // info.mmio_virt = mmio_map_region(info.mmio_bar.base, info.mmio_bar.size);

    printk("[video] Intel GPU probe stage passed.\n");
    return true;
}