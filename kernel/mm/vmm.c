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

// 核心机关：将任意 64 位虚拟地址映射到物理地址
void map_page(uint64_t vaddr, uint64_t paddr, uint32_t flags) {
    // 强制 4KB 对齐
    vaddr &= 0xFFFFFFFFFFFFF000;
    paddr &= 0xFFFFFFFFFFFFF000;

    // 获取顶级页表 PML4 的地址
    uint64_t* pml4 = (uint64_t*)get_cr3();
    
    // 1. 穿透 PML4
    uint64_t pml4_idx = PML4_INDEX(vaddr);
    if ((pml4[pml4_idx] & PAGE_PRESENT) == 0) {
        uint64_t new_pdpt = pmm_alloc_page();
        if(!new_pdpt) { printk("VMM OOM at PML4\n"); return; }
        memset((void*)new_pdpt, 0, PAGE_SIZE); // 1GB 映射的福利：直接操作物理地址
        pml4[pml4_idx] = new_pdpt | PAGE_PRESENT | PAGE_RW | PAGE_USER;
    }

    // 2. 穿透 PDPT
    uint64_t* pdpt = (uint64_t*)(pml4[pml4_idx] & 0xFFFFFFFFFFFFF000);
    uint64_t pdpt_idx = PDPT_INDEX(vaddr);
    if ((pdpt[pdpt_idx] & PAGE_PRESENT) == 0) {
        uint64_t new_pd = pmm_alloc_page();
        if(!new_pd) { printk("VMM OOM at PDPT\n"); return; }
        memset((void*)new_pd, 0, PAGE_SIZE);
        pdpt[pdpt_idx] = new_pd | PAGE_PRESENT | PAGE_RW | PAGE_USER;
    }

    // 3. 穿透 PD
    uint64_t* pd = (uint64_t*)(pdpt[pdpt_idx] & 0xFFFFFFFFFFFFF000);
    uint64_t pd_idx = PD_INDEX(vaddr);
    if ((pd[pd_idx] & PAGE_PRESENT) == 0) {
        uint64_t new_pt = pmm_alloc_page();
        if(!new_pt) { printk("VMM OOM at PD\n"); return; }
        memset((void*)new_pt, 0, PAGE_SIZE);
        pd[pd_idx] = new_pt | PAGE_PRESENT | PAGE_RW | PAGE_USER;
    }

    // 4. 到达终点 PT，写入物理页框
    uint64_t* pt = (uint64_t*)(pd[pd_idx] & 0xFFFFFFFFFFFFF000);
    uint64_t pt_idx = PT_INDEX(vaddr);
    pt[pt_idx] = paddr | flags | PAGE_PRESENT;

    flush_tlb(vaddr);
}

// 连续映射一片区域
void vmm_alloc_map_region(uint64_t vaddr, uint64_t size, uint32_t flags) {
    if (size == 0) return;

    uint64_t start_vaddr = vaddr & 0xFFFFFFFFFFFFF000;
    uint64_t end_vaddr = ((vaddr + size + 0xFFF) & 0xFFFFFFFFFFFFF000);

    for (uint64_t current_addr = start_vaddr; current_addr < end_vaddr; current_addr += PAGE_SIZE) {
        // 先检查是否已经映射，防止重复分配
        // 为保持简单，这里我们直接信任 map_page 的覆盖能力，或者你可以补全检查逻辑
        uint64_t phys_page = pmm_alloc_page();
        if (phys_page == 0) {
            printk("###NO MEM for vmm_alloc_map_region\n");
            return;
        }
        map_page(current_addr, phys_page, flags);
    }

    asm volatile("invlpg (%0)" ::"r" (vaddr) : "memory");
}