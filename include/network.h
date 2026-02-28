#ifndef _NETWORKS_H
#define _NETWORKS_H

#include <stdint.h>
#include <driver/pci.h>
#include <printk.h>
#include <driver/pci/bridge.h>
#include <mm/vmm.h>
#include <driver/net/rtl8168.h>

void rtl8168_init_mmio(pci_device_t *slot, rtl8168_t *nic);
void rtl8168_init_and_read_mac(pci_device_t *dev, rtl8168_t *nic);

#endif