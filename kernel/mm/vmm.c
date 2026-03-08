#include <mm/vmm.h>

// 获取当前页目录最高层 (PML4) 的物理基址
uint64_t get_cr3() {
    uint64_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    return cr3 & 0xFFFFFFFFFFFFF000; // 清除低 12 位的标志位
}

// 刷新 TLB (让 CPU 忘记旧的映射)
void flush_tlb(uint64_t vaddr) {
    __asm__ volatile("invlpg (%0)" ::"r"(vaddr) : "memory");
}

int vmm_map_page(uint64_t pml4_pa, uint64_t va, uint64_t pa, uint64_t flags) {
    uint64_t upper_flags = PTE_RW;
    if (flags & PTE_US) upper_flags |= PTE_US;

    uint64_t* pte = pt_get_pte(pml4_pa, va, 1, upper_flags);
    if (!pte) {
        printk("VMM: Failed to get PTE for VA 0x%08X%08X\n", (uint32_t)(va >> 32), (uint32_t)(va & 0xFFFFFFFF));
        return 0;
    }

    if (*pte & PTE_P) {
        printk("VMM: Warning - VA 0x%08X%08X is already mapped, skipping\n", (uint32_t)(va >> 32), (uint32_t)(va & 0xFFFFFFFF));
        return 0; // 暂时不允许覆盖
    }

    *pte = (pa & PAGE_MASK) | flags | PTE_P;
    return 1;
}

int vmm_alloc_map_region(uint64_t pml4_pa, uint64_t vaddr, uint64_t size, uint32_t flags) {
    uint64_t start = vaddr & PAGE_MASK;
    uint64_t end = ((vaddr + size + PAGE_SIZE - 1) & PAGE_MASK);

    for (uint64_t addr = start; addr < end; addr += PAGE_SIZE) {
        uint64_t phys_page = pmm_alloc_page();
        if (!phys_page) {
            printk("VMM: Failed to allocate physical page for mapping!\n");
            return 0;
        }
        if (!vmm_map_page(pml4_pa, addr, phys_page, flags)) {
            printk("VMM: Failed to map page at VA 0x%08X%08X\n", (uint32_t)(addr >> 32), (uint32_t)(addr & 0xFFFFFFFF));
            return 0;
        }
    }
    return 1;
}

int vmm_unmap_page(uint64_t pml4_pa, uint64_t va) {
    uint64_t upper_flags = PTE_RW;
    uint64_t* pte = pt_get_pte(pml4_pa, va, 0, upper_flags);
    if (!pte) return 0;
    if (!(*pte & PTE_P)) return 0;

    *pte = 0;
    return 1;
}

