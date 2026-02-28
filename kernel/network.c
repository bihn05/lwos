#include <network.h>
#include <mm/vmm.h>
#include <printk.h>

void rtl8168_init_and_read_mac(pci_device_t *slot, rtl8168_t *nic) {
    nic->pci_dev = slot;
    uint8_t bus = slot->bus;
    uint8_t dev = slot->slot;
    uint8_t func = slot->func;

    printk("Initializing RTL8168 at %02x:%02x.%d...\n", bus, dev, func);

    // ==========================================
    // 1. 唤醒设备 (处理 D3hot 休眠状态)
    // ==========================================
    uint16_t status = pci_config_read_word(bus, dev, func, 0x06);
    if (status & 0x0010) { // 支持 Capabilities List
        uint8_t cap_ptr = pci_config_read_word(bus, dev, func, 0x34) & 0xFF;
        while (cap_ptr != 0 && cap_ptr != 0xFF) {
            uint16_t cap_header = pci_config_read_word(bus, dev, func, cap_ptr);
            if ((cap_header & 0xFF) == 0x01) { // 找到 Power Management
                uint16_t pmcsr = pci_config_read_word(bus, dev, func, cap_ptr + 4);
                if ((pmcsr & 0x03) != 0) {
                    printk("  -> Waking up from D%d state...\n", pmcsr & 0x03);
                    pmcsr &= ~0x03; // 清零低2位，切回 D0 状态
                    pci_config_write_word(bus, dev, func, cap_ptr + 4, pmcsr);
                    for (volatile int i = 0; i < 5000000; i++); // 粗略延时，等待硬件通电
                }
                break;
            }
            cap_ptr = (cap_header >> 8) & 0xFF;
        }
    }

    // ==========================================
    // 2. 开启 Memory Space 与 Bus Master，并严格验证
    // ==========================================
    uint16_t cmd = pci_config_read_word(bus, dev, func, 0x04);
    cmd |= (1 << 2) | (1 << 1); // Bit 2: Bus Master, Bit 1: Memory Space
    pci_config_write_word(bus, dev, func, 0x04, cmd);
    
    // 写后读：实体机上极其关键的一步
    uint16_t verify_cmd = pci_config_read_word(bus, dev, func, 0x04);
    if ((verify_cmd & 0x06) != 0x06) {
        printk("  [!] FATAL ERROR: PCI Controller rejected MMIO/DMA enable! Cmd: 0x%04x\n", verify_cmd);
        return; // 如果赋权失败，后续操作毫无意义
    }

    // 2. 读取 BAR0
    uint32_t bar0 = pci_config_read(bus, dev, func, 0x10);
    
    // 检查最低位，必须是 1 才代表这是 I/O 端口
    if ((bar0 & 1) == 0) {
        printk("  [!] BAR0 is not an I/O port! (0x%08x)\n", bar0);
        return;
    }

    // 掩掉低 2 位标志位，得到真正的 I/O 基址 (通常是一个 16 位的端口号，比如 0x2000)
    uint16_t io_base = bar0 & 0xFFFC;
    printk("  -> RTL8168 I/O Base Port: 0x%04x\n", io_base);

    // ==========================================
    // 3. 通过 I/O 端口复位网卡
    // ==========================================
    printk("  -> Resetting via I/O Port...\n");
    outb(io_base + 0x37, 0x10); // 写复位指令
    
    int timeout = 10000;
    while ((inb(io_base + 0x37) & 0x10) && timeout > 0) {
        timeout--;
        for (volatile int i = 0; i < 1000; i++);
    }
    
    if (timeout == 0) {
        printk("  [!] I/O Reset Timeout! The card is truly unresponsive.\n");
    } else {
        printk("  -> I/O Reset COMPLETE! Hardware is ALIVE!\n");
    }

    // ==========================================
    // 4. 通过 I/O 端口读取 MAC 地址
    // ==========================================
    for (int i = 0; i < 6; i++) {
        nic->mac_addr[i] = inb(io_base + i);
    }

    printk("  => MAC via I/O: %02x:%02x:%02x:%02x:%02x:%02x\n",
           nic->mac_addr[0], nic->mac_addr[1], nic->mac_addr[2],
           nic->mac_addr[3], nic->mac_addr[4], nic->mac_addr[5]);

    // ==========================================
    // 4. 构建 DMA 描述符环并通知网卡
    // ==========================================
    if (!rtl8168_init_rings(nic)) {
        return; // 分配内存失败，直接退出
    }

    // ==========================================
    // 5. 配置接收规则，并最终开启 Rx/Tx 引擎
    // ==========================================
    uint16_t io = nic->io_base;
    outb(0xC0, io + 0x50);

    // 允许接收单播(物理匹配)和广播包，允许超大帧
    outl(0x0E0A, io + 0x44); 
    // 设置接收最大包长
    outw(16383, io + 0xDA);

    // 开启 MAC 层的 Rx 和 Tx
    outb(0x0C, io + 0x37); 

    outb(0x00, io + 0x50); // 锁定

    printk("  => RTL8168 is fully operational via PIO and DMA!\n");
}