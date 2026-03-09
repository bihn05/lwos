#include <mm/mmio.h>

static uint64_t g_mmio_next = MMIO_VIRT_BASE;

#define PAGE_SIZE   0x1000ULL
#define PAGE_MASK   0xFFFFFFFFFFFFF000ULL

#define PTE_P       (1ULL << 0)
#define PTE_RW      (1ULL << 1)
#define PTE_PWT     (1ULL << 3)
#define PTE_PCD     (1ULL << 4)

extern int vmm_map_page(uint64_t pml4_pa, uint64_t va, uint64_t pa, uint64_t flags);

void *mmio_map_region(uint64_t pm4_pa, uint64_t phys_base, uint64_t size, uint64_t flags) {
    uint64_t phys_aligned = phys_base & PAGE_MASK;
    uint64_t offset       = phys_base - phys_aligned;
    uint64_t total_size   = offset + size;
    uint64_t page_count   = (total_size + PAGE_SIZE - 1) / PAGE_SIZE;

    uint64_t virt_base = (g_mmio_next + PAGE_SIZE - 1) & PAGE_MASK;

    for (uint64_t i = 0; i < page_count; i++) {
        uint64_t pa = phys_aligned + i * PAGE_SIZE;
        uint64_t va = virt_base   + i * PAGE_SIZE;

        // 对 MMIO，至少建议 PCD=1，避免当普通 RAM 缓存
        if (!vmm_map_page(pm4_pa, va, pa, PTE_RW | PTE_PCD)) {
            return NULL;
        }
    }

    g_mmio_next = virt_base + page_count * PAGE_SIZE;
    return (void *)(uintptr_t)(virt_base + offset);
}

uint8_t mmio_read8(volatile void *base, uint64_t off) {
    return *(volatile uint8_t *)((volatile uint8_t *)base + off);
}

uint16_t mmio_read16(volatile void *base, uint64_t off) {
    return *(volatile uint16_t *)((volatile uint8_t *)base + off);
}

uint32_t mmio_read32(volatile void *base, uint64_t off) {
    return *(volatile uint32_t *)((volatile uint8_t *)base + off);
}

uint64_t mmio_read64(volatile void *base, uint64_t off) {
    return *(volatile uint64_t *)((volatile uint8_t *)base + off);
}

void mmio_write8(volatile void *base, uint64_t off, uint8_t val) {
    *(volatile uint8_t *)((volatile uint8_t *)base + off) = val;
}

void mmio_write16(volatile void *base, uint64_t off, uint16_t val) {
    *(volatile uint16_t *)((volatile uint8_t *)base + off) = val;
}

void mmio_write32(volatile void *base, uint64_t off, uint32_t val) {
    *(volatile uint32_t *)((volatile uint8_t *)base + off) = val;
}

void mmio_write64(volatile void *base, uint64_t off, uint64_t val) {
    *(volatile uint64_t *)((volatile uint8_t *)base + off) = val;
}