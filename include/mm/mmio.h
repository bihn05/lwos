#ifndef _MMIO_H
#define _MMIO_H

#include <stdint.h>
#include <driver/pci.h>
#include <mm.h>

#define MMIO_VIRT_BASE   0xFFFFFFFFD0000000ULL
#define MMIO_VIRT_SIZE   0x01000000ULL   // 16 MiB

void *mmio_map_region(uint64_t pm4_pa, uint64_t phys_base, uint64_t size, uint64_t flags);

uint8_t  mmio_read8 (volatile void *base, uint64_t off);
uint16_t mmio_read16(volatile void *base, uint64_t off);
uint32_t mmio_read32(volatile void *base, uint64_t off);
uint64_t mmio_read64(volatile void *base, uint64_t off);

void mmio_write8 (volatile void *base, uint64_t off, uint8_t  val);
void mmio_write16(volatile void *base, uint64_t off, uint16_t val);
void mmio_write32(volatile void *base, uint64_t off, uint32_t val);
void mmio_write64(volatile void *base, uint64_t off, uint64_t val);

#endif