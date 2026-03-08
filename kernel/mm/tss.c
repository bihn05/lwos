#include <mm.h>
#include <descript.h>
#include <mm/proc.h>

void tss_late_init(uint64_t pml4_pa) {
    uint64_t kstack_base = KERNEL_STACK_REGION_BASE;
    uint64_t kstack_top  = kstack_base + KERNEL_STACK_SIZE;

    for (uint64_t off = 0; off < KERNEL_STACK_SIZE; off += PAGE_SIZE) {
        uint64_t phys = pmm_alloc_page();
        if (!phys) {
            printk("Failed to allocate kernel stack page\n");
            return;
        }

        memset((void*)(uintptr_t)phys, 0, PAGE_SIZE);   // 先清物理页
        if (!vmm_map_page(pml4_pa, kstack_base + off, phys, PTE_RW)) {
            printk("Failed to map kernel stack page va=%p\n", kstack_base + off);
            return;
        }
    }

    for (uint64_t off = 0; off < KERNEL_STACK_SIZE; off += PAGE_SIZE) {
        pt_debug_walk(pml4_pa, kstack_base + off);
    }

    memset(&kernel_tss, 0, sizeof(kernel_tss));
    kernel_tss.rsp0 = kstack_top;
    kernel_tss.iomap_base = sizeof(kernel_tss);

    gdt_set_tss(5, (uint64_t)&kernel_tss, sizeof(kernel_tss) - 1);

    uint16_t tss_sel = 0x28;
    asm volatile("ltr %0" : : "rm"(tss_sel));

    printk("TSS.rsp0=%p\n", kernel_tss.rsp0);
}