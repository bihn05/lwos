#include <driver/pci.h>
#include <printk.h>

uint32_t pci_config_read(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address;
    uint32_t lbus  = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;
 
    address = (uint32_t)((lbus << 16) | (lslot << 11) |
              (lfunc << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
 
    outl(address, 0xCF8);
    return inl(0xCFC);
}
uint16_t pci_config_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    // 先读出 32 位
    uint32_t data = pci_config_read(bus, slot, func, offset & 0xFC);
    // 根据偏移量选择高 16 位或低 16 位
    // (offset & 2) * 8 在偏移 0 时为 0，在偏移 2 时为 16
    return (uint16_t)((data >> ((offset & 2) * 8)) & 0xFFFF);
}
void pci_config_write(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value) {
    uint32_t address;
    uint32_t lbus  = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;
 
    // 构造地址，确保最高位 (Enable bit) 为 1
    address = (uint32_t)((lbus << 16) | (lslot << 11) |
              (lfunc << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
 
    outl(address, 0xCF8);
    outl(value, 0xCFC);
}
void pci_config_write_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t value) {
    // 1. 先读出原本的 32 位数据
    uint32_t old_data = pci_config_read(bus, slot, func, offset & 0xFC);
    
    // 2. 根据偏移量计算位移
    uint32_t shift = (offset & 2) * 8;
    
    // 3. 清除目标区域的数据 (与上 0x0000FFFF 或 0xFFFF0000)
    uint32_t mask = ~(0xFFFF << shift);
    uint32_t new_data = (old_data & mask) | ((uint32_t)value << shift);
    
    // 4. 写回
    pci_config_write(bus, slot, func, offset & 0xFC, new_data);
}
void pci_check_all_buses(void) {
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t device = 0; device < 32; device++) {
            pci_check_device(bus, device);
        }
    }
}
void pci_check_device(uint8_t bus, uint8_t slot) {
    uint8_t function = 0;
    uint16_t vendor_id = pci_config_read_word(bus, slot, function, 0);

    if (vendor_id == 0xFFFF) return; // 插槽为空

    // 检查第一个功能
    pci_check_function(bus, slot, function);

    // 检查 Header Type 确定是否为多功能设备
    uint16_t header_type = pci_config_read_word(bus, slot, function, 0x0C) & 0xFF;
    if (header_type & 0x80) {
        // 多功能设备，扫描剩余的 7 个功能
        for (function = 1; function < 8; function++) {
            if (pci_config_read_word(bus, slot, function, 0) != 0xFFFF) {
                pci_check_function(bus, slot, function);
            }
        }
    }
}
void pci_check_function(uint8_t bus, uint8_t slot, uint8_t func) {
    uint16_t vendor_id = pci_config_read_word(bus, slot, func, 0x00);
    uint16_t device_id = pci_config_read_word(bus, slot, func, 0x02);
    
    // Class Code 在偏移 0x08 的高 16 位
    uint32_t class_reg = pci_config_read(bus, slot, func, 0x08);
    uint8_t class_code = (class_reg >> 24) & 0xFF;
    uint8_t subclass   = (class_reg >> 16) & 0xFF;

    printk("[PCI] %02x:%02x.%d | ID: %04x:%04x | Class: %02x (Sub: %02x)\n",
           bus, slot, func, vendor_id, device_id, class_code, subclass);

    // 如果是显卡 (Class 03)，额外重点标记
    if (class_code == 0x03) {
        printk("  -> Found Graphics Controller!\n");
        // 这里后续可以调用获取 BAR 的逻辑
    }
}
uint64_t pci_get_bar_size(uint8_t b, uint8_t s, uint8_t f, uint8_t bar_idx) {
    uint8_t offset = 0x10 + (bar_idx * 4);
    uint32_t old_val = pci_config_read(b, s, f, offset);
    
    // 写入全 1 来探测
    pci_config_write(b, s, f, offset, 0xFFFFFFFF);
    uint32_t res = pci_config_read(b, s, f, offset);
    
    // 还原
    pci_config_write(b, s, f, offset, old_val);
    
    if (res == 0) return 0;
    
    // 掩掉低位标志（Memory BAR 的低 4 位是标志）
    uint32_t size_mask = res & 0xFFFFFFF0;
    // 计算大小：取反加 1
    return ~size_mask + 1;
}

bool pci_find_device_by_class(uint8_t target_class, uint8_t target_subclass, pci_device_t *out_dev) {
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            
            // 1. 读取偏移 0x00，获取 Vendor ID 判断插槽是否为空
            uint32_t reg0 = pci_config_read(bus, slot, 0, 0x00);
            uint16_t vendor_id = reg0 & 0xFFFF;
            if (vendor_id == 0xFFFF) {
                continue; // 设备不存在
            }

            // 2. 读取偏移 0x0C，获取 Header Type 判断是否为多功能设备
            uint32_t regC = pci_config_read(bus, slot, 0, 0x0C);
            uint8_t header_type = (regC >> 16) & 0xFF;
            int max_func = (header_type & 0x80) ? 8 : 1;

            // 3. 遍历功能 (Function)
            for (uint8_t func = 0; func < max_func; func++) {
                // 如果是多功能设备，需要再次确认当前 function 是否有效
                if (func > 0) {
                    uint32_t f_reg0 = pci_config_read(bus, slot, func, 0x00);
                    if ((f_reg0 & 0xFFFF) == 0xFFFF) {
                        continue;
                    }
                }

                // 4. 读取偏移 0x08，获取 Class Code 和 Subclass
                uint32_t class_reg = pci_config_read(bus, slot, func, 0x08);
                uint8_t class_code = (class_reg >> 24) & 0xFF;
                uint8_t subclass   = (class_reg >> 16) & 0xFF;

                // 5. 匹配目标类别
                if (class_code == target_class && (target_subclass == 0xFF || subclass == target_subclass)) {
                    out_dev->bus = bus;
                    out_dev->slot = slot;
                    out_dev->func = func;
                    
                    // 重新读取一次 reg0 以确保拿到了当前 func 的 ID
                    uint32_t final_reg0 = pci_config_read(bus, slot, func, 0x00);
                    out_dev->vendor_id  = final_reg0 & 0xFFFF;
                    out_dev->device_id  = (final_reg0 >> 16) & 0xFFFF;
                    out_dev->class_code = class_code;
                    out_dev->subclass   = subclass;
                    
                    return true; // 找到即返回
                }
            }
        }
    }
    return false; // 遍历结束未找到
}

pci_bar_info_t pci_read_bar(uint8_t bus, uint8_t slot, uint8_t func, uint8_t bar_index) {
    pci_bar_info_t info = {0};
    uint8_t off = 0x10 + bar_index * 4;

    uint32_t orig = pci_config_read(bus, slot, func, off);
    if (orig == 0 || orig == 0xFFFFFFFF) {
        return info;
    }

    if (orig & 0x1) {
        info.is_io = true;
        info.base = (uint64_t)(orig & ~0x3U);

        pci_config_write(bus, slot, func, off, 0xFFFFFFFF);
        uint32_t sized = pci_config_read(bus, slot, func, off);
        pci_config_write(bus, slot, func, off, orig);

        info.size = (uint32_t)(~(sized & ~0x3U) + 1);
        return info;
    }

    info.is_io = false;
    uint32_t type = (orig >> 1) & 0x3;
    info.prefetchable = (orig >> 3) & 1;

    if (type == 0x2) {
        info.is_64 = true;

        uint32_t orig_hi = pci_config_read(bus, slot, func, off + 4);
        info.base = ((uint64_t)orig_hi << 32) | (orig & ~0xFULL);

        pci_config_write(bus, slot, func, off, 0xFFFFFFFF);
        pci_config_write(bus, slot, func, off + 4, 0xFFFFFFFF);
        uint32_t size_lo = pci_config_read(bus, slot, func, off);
        uint32_t size_hi = pci_config_read(bus, slot, func, off + 4);

        pci_config_write(bus, slot, func, off, orig);
        pci_config_write(bus, slot, func, off + 4, orig_hi);

        uint64_t mask = ((uint64_t)size_hi << 32) | (size_lo & ~0xFULL);
        info.size = ~mask + 1;
    } else {
        info.is_64 = false;
        info.base = (uint64_t)(orig & ~0xFULL);

        pci_config_write(bus, slot, func, off, 0xFFFFFFFF);
        uint32_t sized = pci_config_read(bus, slot, func, off);
        pci_config_write(bus, slot, func, off, orig);

        info.size = (uint32_t)(~(sized & ~0xFULL) + 1);
    }

    return info;
}

void pci_enable_device_mem(uint8_t bus, uint8_t slot, uint8_t func) {
    uint32_t cmdsts = pci_config_read(bus, slot, func, 0x04);
    uint16_t cmd = (uint16_t)(cmdsts & 0xFFFF);

    cmd |= (1 << 1); // Memory Space Enable
    cmd |= (1 << 2); // Bus Master Enable (可选，但一般顺手开)
    // I/O space 不是必须，除非你用 0x1CE/0x1CF 端口
    cmd |= (1 << 0);

    cmdsts = (cmdsts & 0xFFFF0000U) | cmd;
    pci_config_write(bus, slot, func, 0x04, cmdsts);
}

bool bga_find_pci_device(pci_addr_t *out) {
    for (uint16_t bus = 0; bus < 256; ++bus) {
        for (uint8_t slot = 0; slot < 32; ++slot) {
            for (uint8_t func = 0; func < 8; ++func) {
                uint32_t id = pci_config_read(bus, slot, func, 0x00);
                if (id == 0xFFFFFFFF)
                    continue;

                uint16_t vendor = (uint16_t)(id & 0xFFFF);
                uint16_t device = (uint16_t)((id >> 16) & 0xFFFF);

                if (vendor == BGA_PCI_VENDOR_ID && device == BGA_PCI_DEVICE_ID) {
                    out->bus  = (uint8_t)bus;
                    out->slot = slot;
                    out->func = func;
                    return true;
                }
            }
        }
    }
    return false;
}

uint16_t pci_get_vendor_id(uint8_t bus, uint8_t slot, uint8_t func) {
    uint32_t val = pci_config_read(bus, slot, func, 0x00);
    return (uint16_t)(val & 0xFFFF);
}

uint16_t pci_get_device_id(uint8_t bus, uint8_t slot, uint8_t func) {
    uint32_t val = pci_config_read(bus, slot, func, 0x00);
    return (uint16_t)((val >> 16) & 0xFFFF);
}

uint16_t pci_addr_vendor_id(pci_addr_t addr) {
    return pci_get_vendor_id(addr.bus, addr.slot, addr.func);
}

uint16_t pci_addr_device_id(pci_addr_t addr) {
    return pci_get_device_id(addr.bus, addr.slot, addr.func);
}
