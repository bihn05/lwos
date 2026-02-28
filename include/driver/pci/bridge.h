#ifndef _PCI_BRIDGE_H
#define _PCI_BRIDGE_H

#include <driver/pci.h>
#include <driver/port.h>

void pci_find_and_inspect_bridge_for_bus(uint8_t target_bus);

#endif