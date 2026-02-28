#include <driver/net/rtl8168.h>

bool rtl8168_init_rings(rtl8168_t *nic) {
    // 1. 分配物理内存 (9 个 4KB 页 = 36KB)
    // 包含 1 页描述符 + 8 页 Rx 缓冲区 (16 * 2KB)
    uint64_t dma_phys = pmm_alloc_contiguous_pages(9);
    if (dma_phys == 0) {
        printk("  [!] Failed to allocate contiguous memory for DMA!\n");
        return false;
    }

    // 2. 映射虚拟地址并禁用 Cache (极度重要)
    // 假设你的 vmm 提供恒等映射或高半核映射，这里假设 vaddr = dma_phys
    uint64_t vaddr = dma_phys;
    vmm_alloc_map_region(vaddr, 9 * 4096, PAGE_PRESENT | PAGE_RW | PAGE_DISABLE_CACHE);

    // 3. 在虚拟内存上划分区域
    nic->rx_ring = (rtl8168_desc_t *)vaddr;
    nic->tx_ring = (rtl8168_desc_t *)(vaddr + 256); // 16个描述符占 256 字节
    nic->rx_buffers = (uint8_t *)(vaddr + 4096);    // 第二页开始是接收缓冲区
    
    nic->current_rx = 0;
    nic->current_tx = 0;

    // 4. 初始化 Rx 描述符环
    for (int i = 0; i < NUM_RX_DESC; i++) {
        uint64_t buf_phys = dma_phys + 4096 + (i * RX_BUFFER_SIZE);
        
        nic->rx_ring[i].buf_addr_low  = (uint32_t)(buf_phys & 0xFFFFFFFF);
        nic->rx_ring[i].buf_addr_high = (uint32_t)(buf_phys >> 32);
        nic->rx_ring[i].vlan          = 0;
        
        // 设置缓冲区大小，并将所有权 (OWN) 交给网卡
        nic->rx_ring[i].command = RX_BUFFER_SIZE | DESC_OWN;
        
        // 最后一个描述符打上 EOR (绕回) 标记
        if (i == NUM_RX_DESC - 1) {
            nic->rx_ring[i].command |= DESC_EOR;
        }
    }

    // 5. 初始化 Tx 描述符环 (默认清空，OWN=0 表示 CPU 拥有)
    for (int i = 0; i < NUM_TX_DESC; i++) {
        nic->tx_ring[i].command = 0;
        nic->tx_ring[i].vlan = 0;
        nic->tx_ring[i].buf_addr_low = 0;
        nic->tx_ring[i].buf_addr_high = 0;
        
        if (i == NUM_TX_DESC - 1) {
            nic->tx_ring[i].command |= DESC_EOR;
        }
    }

    // 6. 将环的【物理地址】通过 PIO 写入网卡寄存器
    uint16_t io = nic->io_base;
    outb(io + 0x50, 0xC0); // 解锁配置寄存器
    
    // 写入 Rx Ring 物理基址 (偏移 0xE4)
    outl(io + 0xE4, (uint32_t)(dma_phys & 0xFFFFFFFF));
    outl(io + 0xE8, (uint32_t)(dma_phys >> 32));
    
    // 写入 Tx Ring 物理基址 (偏移 0x20) - 这是高优先级发送环
    // 注意: dma_phys + 256 是 Tx Ring 的物理地址
    uint64_t tx_phys = dma_phys + 256;
    outl(io + 0x20, (uint32_t)(tx_phys & 0xFFFFFFFF));
    outl(io + 0x24, (uint32_t)(tx_phys >> 32));

    outb(io + 0x50, 0x00); // 重新锁定

    printk("  -> DMA descriptor rings configured successfully.\n");
    return true;
}
void rtl8168_poll_rx(rtl8168_t *nic) {
    uint16_t curr = nic->current_rx;
    rtl8168_desc_t *desc = &nic->rx_ring[curr];

    // 检查 OWN 位。
    // 如果最高位是 0，说明网卡已经写完数据，把描述符控制权交还给了 CPU
    if ((desc->command & DESC_OWN) == 0) {
        
        // 1. 获取这个数据包的实际大小 (低 14 位)
        uint32_t pkt_len = desc->command & 0x3FFF;
        
        // 2. 计算这个包在虚拟内存中的地址
        // 之前我们将所有的 Rx Buffer 连续分配在 nic->rx_buffers 开始的地方
        // 每个 Buffer 的大小是固定的 RX_BUFFER_SIZE (2048)
        uint8_t *packet = nic->rx_buffers + (curr * RX_BUFFER_SIZE);

        // 3. 打印收包信息
        printk("\n>>> [RTL8168] Packet Received! Length: %d bytes (Desc: %d)\n", pkt_len, curr);

        // 以太网帧头至少有 14 个字节 (6字节目标MAC + 6字节源MAC + 2字节类型)
        if (pkt_len >= 14) {
            printk("    Dest MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                   packet[0], packet[1], packet[2], packet[3], packet[4], packet[5]);
            
            printk("    Src MAC : %02x:%02x:%02x:%02x:%02x:%02x\n",
                   packet[6], packet[7], packet[8], packet[9], packet[10], packet[11]);
            
            // EtherType 是大端序 (Big-Endian)，需要拼合一下
            uint16_t eth_type = (packet[12] << 8) | packet[13];
            printk("    Type    : 0x%04x\n", eth_type);
            
            // 常见的 Type 提示：
            // 0x0800 = IPv4
            // 0x0806 = ARP
            // 0x86DD = IPv6
        }

        // ==========================================
        // 4. 将描述符的控制权还给网卡 (极其重要！)
        // ==========================================
        // 重置缓冲区大小，并将 OWN 位设为 1
        uint32_t new_cmd = RX_BUFFER_SIZE | DESC_OWN;
        
        // 如果这是环的最后一个描述符，千万别忘了保留 EOR (绕回) 标志
        if (curr == NUM_RX_DESC - 1) {
            new_cmd |= DESC_EOR;
        }
        
        desc->command = new_cmd;

        // 5. 游标向前推进，准备下一次轮询下一个描述符
        nic->current_rx = (curr + 1) % NUM_RX_DESC;
    }
}