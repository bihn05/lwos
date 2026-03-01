#ifndef _RTL8168_H
#define _RTL8168_H

#include <stdint.h>
#include <driver/pci.h>
#include <printk.h>
#include <driver/pci/bridge.h>
#include <mm.h>

// RTL8168 描述符结构 (16字节 / 128位对齐)
// 必须确保编译器不进行结构体填充 (__attribute__((packed)))
typedef struct {
    uint32_t command;       // 控制位和数据包长度
    uint32_t vlan;          // VLAN 信息 (我们不需要，填 0)
    uint32_t buf_addr_low;  // 缓冲区的物理地址低 32 位
    uint32_t buf_addr_high; // 缓冲区的物理地址高 32 位
} __attribute__((packed)) rtl8168_desc_t;

// 扩展你的网络设备结构体
typedef struct {
    pci_device_t *pci_dev;
    uint16_t io_base;       // PIO 基址
    uint8_t  mac_addr[6];

    // DMA 相关的虚拟地址 (供 CPU 读写)
    rtl8168_desc_t *rx_ring;
    rtl8168_desc_t *tx_ring;
    uint8_t        *rx_buffers;
    uint8_t        *tx_buffers; // 可选，如果 Tx 数据也预先分配
    
    uint16_t current_rx;    // CPU 当前读取到哪个 Rx 描述符
    uint16_t current_tx;    // CPU 当前准备写哪个 Tx 描述符
} rtl8168_t;

// Command 寄存器的关键标志位
#define DESC_OWN  (1 << 31) // 1 = 网卡拥有 (CPU不能动), 0 = CPU拥有 (网卡写完了)
#define DESC_EOR  (1 << 30) // End of Ring (环的最后一个描述符，网卡看到它会绕回开头)
#define DESC_FS   (1 << 29) // First Segment (包的起始)
#define DESC_LS   (1 << 28) // Last Segment (包的结束)

// 我们设定环的大小，单线程轮询不需要太大，16 到 32 个足够了
#define NUM_RX_DESC 128
#define NUM_TX_DESC 16
#define RX_BUFFER_SIZE 2048 // 以太网帧最大 1518，2KB 足够

bool rtl8168_init_rings(rtl8168_t *nic);
void rtl8168_poll_rx(rtl8168_t *nic);

#endif