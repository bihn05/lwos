#include <driver/net/rtl8168.h>
#include <tools.h>

bool rtl8168_init_rings(rtl8168_t *nic) {
    // 1. 分配物理内存

    // 计算需要的总页数
    // 1. 描述符占用的页数 (128*16*2 / 4096 = 1)
    uint64_t desc_pages = (NUM_RX_DESC * 16 + NUM_TX_DESC * 16 + 4095) / 4096;
    // 2. 缓冲区占用的页数 (128*2048 / 4096 = 64)
    uint64_t buff_pages = (NUM_RX_DESC * RX_BUFFER_SIZE + 4095) / 4096;
    int64_t total_pages = desc_pages + buff_pages;

    printk("!!-> ALLOCATED PAGE : %d PAGES\n", total_pages);

    // 分配连续物理页 (现在是 65 页)
    uint64_t dma_phys = pmm_alloc_contiguous_pages(total_pages);

    printk("  -> dma_phys = 0x%08x%08x\n", (uint32_t)(dma_phys >> 32), (uint32_t)(dma_phys & 0xFFFFFFFF));
    if (dma_phys == 0) {
        printk("  [!] Failed to allocate contiguous memory for DMA!\n");
        return false;
    }

    // 2. 映射虚拟地址并禁用 Cache (极度重要)
    // 假设你的 vmm 提供恒等映射或高半核映射，这里假设 vaddr = dma_phys
    uint64_t vaddr = dma_phys;
    vmm_alloc_map_region(vaddr, total_pages * 4096, PAGE_PRESENT | PAGE_RW | PAGE_DISABLE_CACHE);

    // 3. 在虚拟内存上划分区域
    nic->rx_ring = (rtl8168_desc_t *)vaddr;
    nic->tx_ring = (rtl8168_desc_t *)(vaddr + 256); // 16个描述符占 256 字节
    nic->rx_buffers = (uint8_t *)(vaddr + 4096);    // 第二页开始是接收缓冲区

    printk("  -> rx_ring = 0x%08x%08x\n", (uint32_t)(vaddr >> 32), (uint32_t)(vaddr & 0xFFFFFFFF));
    
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
    outb(0xC0, io + 0x50); // 解锁配置寄存器

    outb(0x00, io + 0x52);

    uint8_t conf2 = inb(io + 0x53);
    outb(conf2 | 0x01, io + 0x53);

    uint8_t conf3 = inb(io + 0x54);
    outb(conf3 | 0x10, io + 0x54);
    
    // 写入 Rx Ring 物理基址 (偏移 0xE4)
    outl((uint32_t)(dma_phys >> 32), io + 0xE8);
    outl((uint32_t)(dma_phys & 0xFFFFFFFF), io + 0xE4);
    
    // 写入 Tx Ring 物理基址 (偏移 0x20) - 这是高优先级发送环
    // 注意: dma_phys + 256 是 Tx Ring 的物理地址
    uint64_t tx_phys = dma_phys + 256;

    outl((uint32_t)(tx_phys >> 32), io + 0x24);
    outl((uint32_t)(tx_phys & 0xFFFFFFFF), io + 0x20);

    outb(0x0C, io + 0x37);
    outb(0x00, io + 0x50); // 重新锁定

    uint8_t phystat = inb(nic->io_base + 0x6C);
    printk("PHY Status: 0x%02x (LinkUp: %d)\n", phystat, (phystat >> 1) & 1);

    // 1. 确保接收/发送引擎暂时关闭，以便更新基址
    outb(0x00, nic->io_base + 0x37); 

    // 2. 重新加载描述符基地址 (RDSAR)
    outl((uint32_t)(dma_phys >> 32), io + 0xE8);
    outl((uint32_t)(dma_phys & 0xFFFFFFFF), io + 0xE4);

    // 3. 开启接收/发送引擎，这将触发硬件读取第一个描述符
    outb(0x0C, nic->io_base + 0x37); 

    // 4. (关键) 对于某些版本的 8168，需要往 TPPOLL 寄存器写一个位来强制开始扫描
    // 偏移 0x38 是 Transmit Priority Polling，Rx 有时也依赖这个初始化的心跳
    outb(0x40, nic->io_base + 0x38); // 设置高优先级轮询

    printk("  -> DMA descriptor rings configured successfully.\n");
    return true;
}
void rtl8168_poll_rx(rtl8168_t *nic) {
    outw(0xFFFF, nic->io_base + 0x3E);
    uint16_t isr = inw(nic->io_base + 0x3E);
    if (isr & 0x0010) {
        outw(0x0010, nic->io_base + 0x3E);
        outb(0x0C, nic->io_base + 0x37);
    }

    uint16_t curr = nic->current_rx;
    rtl8168_desc_t *desc = &nic->rx_ring[curr];

    asm volatile ("" ::: "memory");

    // printk("Polling desc %d, command: 0x%08x\n", curr, desc->command);

    // 检查 OWN 位。
    // 如果最高位是 0，说明网卡已经写完数据，把描述符控制权交还给了 CPU
    if ((desc->command & DESC_OWN) == 0) {
        uint32_t pkt_len = desc->command & 0x3FFF;

        if (pkt_len < 14) {
            goto reset_desc;
        }

        uint8_t *packet = nic->rx_buffers + (curr * RX_BUFFER_SIZE);

        // 打印简洁的报头
        printk("\n[ETH] Received: %d bytes | Desc: %d\n", pkt_len, curr);
        
        // 打印以太网核心信息
        uint16_t eth_type = (packet[12] << 8) | packet[13];
        printk("      Src: %02x:%02x:%02x:%02x:%02x:%02x -> Dst: %02x:%02x:%02x:%02x:%02x:%02x | Type: 0x%04x\n",
               packet[6], packet[7], packet[8], packet[9], packet[10], packet[11],
               packet[0], packet[1], packet[2], packet[3], packet[4], packet[5],
               eth_type);

        // 如果是 ARP (0x0806)
        if (eth_type == 0x0806) {
            printk("      [ARP] This is likely the host looking for an IP address!\n");
        } 
        // 如果是 IPv4 (0x0800)
        else if (eth_type == 0x0800) {
            printk("      [IPv4] Protocol: %d\n", packet[23]); // 1=ICMP, 6=TCP, 17=UDP
        }
        else if (eth_type == 0x88B5) {
            printk("      [LWOS] Received command.\n");
            dump_chunk(packet, 1);
        }

        // ==========================================
        // 重要：重置并交还描述符
        // ==========================================
        reset_desc:
        uint32_t reset_cmd = RX_BUFFER_SIZE | DESC_OWN;
        if (curr == 127) {
            printk("  -> end of ring (current rx idx = %d)\n", curr);
            reset_cmd |= DESC_EOR;
        }
        desc->command = reset_cmd;
        printk("  -> return desc command : 0x%08x\n", reset_cmd);

        nic->current_rx = (curr + 1) % NUM_RX_DESC;

        outb(0x0C, nic->io_base + 0x37);
    }
}