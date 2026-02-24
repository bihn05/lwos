#ifndef _MAP_H_
#define _MAP_H_

#include <stdint.h>
#include <string.h>
#include <kernel.h>
#include <mm/pmm.h>

#define PAGE_PRESENT 0x1
#define PAGE_RW 0x2
#define PAGE_USER 0x4

#define PT_BASE_VADDR 0xffc00000
#define PD_VADDR 0xfffff000

#define PD_INDEX(addr) ((addr) >> 22)
#define PT_INDEX(addr) (((addr) >> 12) & 0x3ff)

extern void flush_tlb(uint32_t address);

void map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags) {
    // must be aligned to 4kb
    uint32_t v_aligned = virtual_addr & 0xfffff000;
    uint32_t p_aligned = physical_addr & 0xfffff000;

    uint32_t pd_idx = PD_INDEX(v_aligned);
    uint32_t pt_idx = PT_INDEX(v_aligned);

    // get pd ptr
    uint32_t* pd = (uint32_t*)PD_VADDR;

    // check present
    if ((pd[pd_idx] & PAGE_PRESENT) == 0) {
        uint32_t new_pt_phys = pmm_alloc_page();
        if (new_pt_phys == 0) {
            // no mem
            printk("###NO MEM\n");
            return;
        }
        pd[pd_idx] = new_pt_phys | PAGE_PRESENT | PAGE_RW | PAGE_USER;

        __asm volatile ("mov %cr3, %eax");
        __asm volatile ("mov %eax, %cr3");

        uint32_t* pt_vaddr = (uint32_t*)(PT_BASE_VADDR + (pd_idx * 4096));

        memset(pt_vaddr, 0, 4096);
    }

    // get pt ptr v
    uint32_t* pt = (uint32_t*)(PT_BASE_VADDR + (pd_idx * 4096));

    // write down pte
    pt[pt_idx] = p_aligned | flags;

    // flush tlb
    flush_tlb(v_aligned);
}
void unmap_page(uint32_t virtual_addr) {
    uint32_t pd_idx = PD_INDEX(virtual_addr);
    uint32_t pt_idx = PT_INDEX(virtual_addr);

    // get pd ptr
    uint32_t* pd = (uint32_t*)PD_VADDR;

    // if not present, return directly
    if ((pd[pd_idx] & PAGE_PRESENT) == 0) {
        return;
    }

    // get pt vaddr
    uint32_t* pt = (uint32_t*)(PT_BASE_VADDR + (pd_idx * 4096));

    // set unpresent
    pt[pt_idx] = 0;

    // flush tlb
    flush_tlb(virtual_addr);
}
void vmm_init() {
    uint32_t* pd_phys = (uint32_t*)0x100000;
    pd_phys[1023] = 0x100000 | PAGE_PRESENT | PAGE_RW;
    __asm volatile ("mov %cr3, %eax");
    __asm volatile ("mov %eax, %cr3");
}
void vmm_alloc_map_region(uint32_t vaddr, uint32_t size, uint32_t flags) {
    if (size == 0) return;

    uint32_t start_vaddr = vaddr & 0xfffff000;
    uint32_t end_vaddr = ((vaddr + size + 0xfff) & 0xfffff000);

    for (uint32_t current_addr = start_vaddr; current_addr < end_vaddr; current_addr += 4096) {

        uint32_t pd_idx = PD_INDEX(current_addr);
        uint32_t pt_idx = PT_INDEX(current_addr);
        uint32_t* pd = (uint32_t*)PD_VADDR;

        int is_mapped = 0;
        if ((pd[pd_idx] & PAGE_PRESENT) != 0) {
            uint32_t* pt = (uint32_t*)(PT_BASE_VADDR + (pd_idx * 4096));
            if ((pt[pt_idx] & PAGE_PRESENT) != 0) {
                is_mapped = 1;
            }
        }

        if (!is_mapped) {
            uint32_t phys_page = pmm_alloc_page();
            if (phys_page == 0) {
                printk("###NO MEM for vmm_alloc_map_region\n");
                return;
            }
            map_page(current_addr, phys_page, flags);
        }

        memset((void*)current_addr, 0, 4096);
    }
}

#endif