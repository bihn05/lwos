#ifndef _BAR_H
#define _BAR_H

#include <stdint.h>
#include <driver/pci.h>

bool pci_get_bar_info(pci_device_t *dev, int bar_index, pci_bar_info_t *out);

#endif