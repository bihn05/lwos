#include <mm/proc.h>

extern uint64_t get_cr3(void);

uint64_t create_user_address_space(void) {
    uint64_t new_pml4_pa = pmm_alloc_page();
    if (!new_pml4_pa) return 0;

    memset(pa_to_ptr(new_pml4_pa), 0, PAGE_SIZE);

    uint64_t* new_pml4 = (uint64_t*)pa_to_ptr(new_pml4_pa);
    uint64_t* kernel_pml4 = (uint64_t*)pa_to_ptr(get_cr3());

    // 只复制高半区内核映射
    for (int i = 256; i < 512; i++) {
        new_pml4[i] = kernel_pml4[i];
    }

    return new_pml4_pa;
}

int map_user_stack(uint64_t pml4_pa, uint64_t stack_top, uint64_t size) {
    uint64_t start = (stack_top - size) & PAGE_MASK;
    uint64_t end   = stack_top & PAGE_MASK;

    for (uint64_t va = start; va < end; va += PAGE_SIZE) {
        uint64_t pa = pmm_alloc_page();
        if (!pa) return 0;

        memset(pa_to_ptr(pa), 0, PAGE_SIZE);

        if (!vmm_map_page(pml4_pa, va, pa, PTE_US | PTE_RW)) {
            return 0;
        }
    }

    return 1;
}

static uint64_t g_next_kstack_slot = 0;

int alloc_thread_kernel_stack(thread_t* th, uint64_t kernel_pml4_pa) {
    uint64_t slot = g_next_kstack_slot++;
    uint64_t base = KERNEL_STACK_REGION_BASE + slot * KERNEL_STACK_STRIDE;
    uint64_t top  = base + KERNEL_STACK_SIZE;

    for (uint64_t off = 0; off < KERNEL_STACK_SIZE; off += PAGE_SIZE) {
        uint64_t pa = pmm_alloc_page();
        if (!pa) return 0;

        memset(pa_to_ptr(pa), 0, PAGE_SIZE);

        if (!vmm_map_page(kernel_pml4_pa, base + off, pa, PTE_RW)) {
            return 0;
        }
    }

    th->kstack_base = base;
    th->kstack_top  = top;
    th->kernel_rsp  = top;
    return 1;
}