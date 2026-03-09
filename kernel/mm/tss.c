#include <mm.h>
#include <descript.h>
#include <mm/proc.h>
tss_t       kernel_tss;
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
static void map_stack_region(uint64_t pml4_pa, uint64_t base, uint64_t size) {
    for (uint64_t off = 0; off < size; off += PAGE_SIZE) {
        uint64_t phys = pmm_alloc_page();
        if (!phys) {
            printk("IST stack alloc failed: va=%p\n", base + off);
            while (1) { }
        }

        // 你当前还依赖低端 identity map，所以先直接清物理页
        memset((void*)(uintptr_t)phys, 0, PAGE_SIZE);

        if (!vmm_map_page(pml4_pa, base + off, phys, PTE_RW)) {
            printk("IST stack map failed: va=%p pa=%p\n", base + off, phys);
            while (1) { }
        }
    }
}
void ist_init(uint64_t pml4_pa) {
    // 1. 映射 Page Fault 专用栈
    map_stack_region(pml4_pa, PAGE_FAULT_STACK_BASE, PAGE_FAULT_STACK_SIZE);

    // 2. 映射 Double Fault 专用栈
    map_stack_region(pml4_pa, DOUBLE_FAULT_STACK_BASE, DOUBLE_FAULT_STACK_SIZE);

    // 3. 写入同一个 kernel_tss 的 IST 表
    kernel_tss.ist[0] = PAGE_FAULT_STACK_TOP;   // IST1 -> #PF
    kernel_tss.ist[1] = DOUBLE_FAULT_STACK_TOP; // IST2 -> #DF

    printk("IST1(#PF) = %p\n", kernel_tss.ist[0]);
    printk("IST2(#DF) = %p\n", kernel_tss.ist[1]);
}
void map_video_buffer(uint64_t pml4_pa) {
    for (uint64_t off = 0; off < 0xA000; off += 0x1000) {
        vmm_map_page(pml4_pa,
                     LEGACY_FB_VIRT_BASE + off,
                     LEGACY_FB_PHYS_BASE + off,
                     PTE_RW);
    }
}