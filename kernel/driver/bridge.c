#include <driver/pci.h>
#include <driver/port.h>
#include <driver/pci/bridge.h>
#include <printk.h>

void pci_find_and_inspect_bridge_for_bus(uint8_t target_bus) {
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t dev = 0; dev < 32; dev++) {
            for (uint8_t func = 0; func < 8; func++) {
                uint32_t vendor = pci_config_read(bus, dev, func, 0x00) & 0xFFFF;
                if (vendor == 0xFFFF) {
                    if (func == 0) break; // 空插槽跳过
                    continue;
                }

                // 读取 Class Code
                uint32_t class_reg = pci_config_read(bus, dev, func, 0x08);
                uint8_t class_code = (class_reg >> 24) & 0xFF;
                uint8_t subclass   = (class_reg >> 16) & 0xFF;

                // 判断是否为 PCI-to-PCI Bridge (0604)
                if (class_code == 0x06 && subclass == 0x04) {
                    // 读取总线号寄存器 (偏移 0x18)
                    // Byte 0: Primary Bus, Byte 1: Secondary Bus, Byte 2: Subordinate Bus
                    uint32_t bus_numbers = pci_config_read(bus, dev, func, 0x18);
                    uint8_t primary_bus   = bus_numbers & 0xFF;
                    uint8_t secondary_bus = (bus_numbers >> 8) & 0xFF;
                    uint8_t sub_bus       = (bus_numbers >> 16) & 0xFF;

                    if (secondary_bus == target_bus || (target_bus >= secondary_bus && target_bus <= sub_bus)) {
                        printk("Found Bridge for Bus %d at %02x:%02x.%d!\n", target_bus, bus, dev, func);
                        
                        // 检查桥接器的 Command 寄存器
                        uint16_t cmd = pci_config_read_word(bus, dev, func, 0x04);
                        printk("  -> Bridge Command Reg: 0x%04x (Mem Enable: %d)\n", cmd, (cmd & 0x02) >> 1);
                        
                        // 检查 Memory Window (偏移 0x20)
                        uint32_t mem_window = pci_config_read(bus, dev, func, 0x20);
                        uint16_t mem_base  = mem_window & 0xFFFF;
                        uint16_t mem_limit = mem_window >> 16;
                        
                        // P2P 桥的内存窗口是对齐到 1MB 的
                        uint32_t actual_base  = (mem_base & 0xFFF0) << 16;
                        uint32_t actual_limit = ((mem_limit & 0xFFF0) << 16) | 0xFFFFF;
                        
                        printk("  -> Bridge Memory Window: 0x%08x - 0x%08x\n", actual_base, actual_limit);

                        printk("  -> Forcing PCI Bridge window open for Bus 8...\n");
                        uint32_t window_val = 0xf200f200;
                        pci_config_write(bus, dev, func, 0x20, window_val);
                        pci_config_write(bus, dev, func, 0x24, window_val);

                        pci_config_write(bus, dev, func, 0x28, 0x00000000);
                        pci_config_write(bus, dev, func, 0x2C, 0x00000000);

                        uint16_t b_cmd = pci_config_read_word(bus, dev, func, 0x04);
                        b_cmd |= (1 << 2) | (1 << 1); 
                        pci_config_write_word(bus, dev, func, 0x04, b_cmd);

                        printk("  -> Bridge window patched! Bus 8 is OPEN.\n");

                        // 检查 Memory Window (偏移 0x20)
                        mem_window = pci_config_read(bus, dev, func, 0x20);
                        mem_base  = mem_window & 0xFFFF;
                        mem_limit = mem_window >> 16;
                        
                        // P2P 桥的内存窗口是对齐到 1MB 的
                        actual_base  = (mem_base & 0xFFF0) << 16;
                        actual_limit = ((mem_limit & 0xFFF0) << 16) | 0xFFFFF;
                        
                        printk("  -> Bridge Memory Window: 0x%08x - 0x%08x\n", actual_base, actual_limit);

                        b_cmd = pci_config_read_word(bus, dev, func, 0x04);
                        printk("  -> Bridge Command Reg: 0x%04x (Mem Enable: %d)\n", b_cmd, (b_cmd & 0x02) >> 1);
                    }
                }
            }
        }
    }
}