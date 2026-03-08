#include <mm/pt.h>
#include <printk.h>

extern uint64_t pmm_alloc_page(void);

static uint64_t* pt_get_table_from_entry(uint64_t entry) {
    if (!(entry & PTE_P)) return NULL;
    if (entry & PTE_PS) return NULL;   // huge page 不是下一层表
    return (uint64_t*)pa_to_ptr(entry_addr(entry));
}

static uint64_t* pt_next_table(uint64_t* table, uint16_t index, int create, uint64_t upper_flags) {
    uint64_t entry = table[index];

    if (entry & PTE_P) {
        if (entry & PTE_PS) {
            if (!create) return NULL;
            if (!split_2m_page(table, index)) return NULL;
            entry = table[index];
        }
        return (uint64_t*)pa_to_ptr(entry_addr(entry));
    }

    if (!create) {
        printk("PT: No table at index %u and create flag is not set\n", index);
        return NULL;
    }

    uint64_t new_pa = pmm_alloc_page();
    if (!new_pa) {
        printk("PT: Failed to allocate new page for table at index %u\n", index);
        return NULL;
    }

    memset(pa_to_ptr(new_pa), 0, PAGE_SIZE);
    table[index] = new_pa | upper_flags | PTE_P;

    return (uint64_t*)pa_to_ptr(new_pa);
}

uint64_t* pt_get_pte(uint64_t pml4_pa, uint64_t va, int create, uint64_t upper_flags) {
    uint64_t* pml4 = (uint64_t*)pa_to_ptr(pml4_pa);
    if (!pml4) {
        printk("PT: Failed to get PML4 table from PA 0x%08X%08X\n", (uint32_t)(pml4_pa >> 32), (uint32_t)(pml4_pa & 0xFFFFFFFF));
        return NULL;
    }

    uint64_t* pdpt = pt_next_table(pml4, pml4_index(va), create, upper_flags);
    if (!pdpt) {
        printk("PT: Failed to get PDPT for VA 0x%08X%08X\n", (uint32_t)(va >> 32), (uint32_t)(va & 0xFFFFFFFF));
        return NULL;
    }

    uint64_t* pd = pt_next_table(pdpt, pdpt_index(va), create, upper_flags);
    if (!pd) {
        printk("PT: Failed to get PD for VA 0x%08X%08X\n", (uint32_t)(va >> 32), (uint32_t)(va & 0xFFFFFFFF));
        return NULL;
    }

    uint64_t* pt = pt_next_table(pd, pd_index(va), create, upper_flags);
    if (!pt) {
        printk("PT: Failed to get PT for VA 0x%08X%08X\n", (uint32_t)(va >> 32), (uint32_t)(va & 0xFFFFFFFF));
        return NULL;
    }

    return &pt[pt_index(va)];
}

int pt_query(uint64_t pml4_pa, uint64_t va, uint64_t* out_pte) {
    uint64_t* pml4 = (uint64_t*)pa_to_ptr(pml4_pa);
    if (!pml4) return -1;

    uint64_t* pdpt = pt_get_table_from_entry(pml4[pml4_index(va)]);
    if (!pdpt) return -1;

    uint64_t* pd = pt_get_table_from_entry(pdpt[pdpt_index(va)]);
    if (!pd) return -1;

    uint64_t* pt = pt_get_table_from_entry(pd[pd_index(va)]);
    if (!pt) return -1;

    *out_pte = pt[pt_index(va)];
    return 0;
}

void pt_debug_walk(uint64_t pml4_pa, uint64_t va) {
    uint64_t* pml4 = (uint64_t*)pa_to_ptr(pml4_pa);

    uint16_t i4 = pml4_index(va);
    uint16_t i3 = pdpt_index(va);
    uint16_t i2 = pd_index(va);
    uint16_t i1 = pt_index(va);

    uint64_t pml4e = pml4[i4];
    printk("VA=%p\n", va);
    printk("PML4E[%u] = %p\n", i4, pml4e);
    if (!(pml4e & PTE_P)) return;

    uint64_t* pdpt = (uint64_t*)pa_to_ptr(entry_addr(pml4e));
    uint64_t pdpte = pdpt[i3];
    printk("PDPTE[%u] = %p\n", i3, pdpte);
    if (!(pdpte & PTE_P)) return;
    if (pdpte & PTE_PS) return;

    uint64_t* pd = (uint64_t*)pa_to_ptr(entry_addr(pdpte));
    uint64_t pde = pd[i2];
    printk("PDE[%u] = %p\n", i2, pde);
    if (!(pde & PTE_P)) return;
    if (pde & PTE_PS) return;

    uint64_t* pt = (uint64_t*)pa_to_ptr(entry_addr(pde));
    uint64_t pte = pt[i1];
    printk("PTE[%u] = %p\n", i1, pte);
}
static int split_2m_page(uint64_t* pd, uint16_t index) {
    uint64_t old_pde = pd[index];
    if (!(old_pde & PTE_P)) return 0;
    if (!(old_pde & PTE_PS)) return 0;

    uint64_t old_base = old_pde & 0xFFFFFFFFFFE00000ULL;

    uint64_t new_pt_pa = pmm_alloc_page();
    if (!new_pt_pa) return 0;

    uint64_t* new_pt = pa_to_ptr(new_pt_pa);
    memset(new_pt, 0, 4096);

    uint64_t leaf_flags = old_pde & (PTE_RW | PTE_US | PTE_PWT | PTE_PCD | PTE_G | PTE_NX);

    for (int i = 0; i < 512; i++) {
        new_pt[i] = (old_base + i * 0x1000ULL) | leaf_flags | PTE_P;
    }

    uint64_t upper_flags = PTE_RW;
    if (old_pde & PTE_US) upper_flags |= PTE_US;

    pd[index] = new_pt_pa | upper_flags | PTE_P;

    asm volatile("invlpg (%0)" : : "r"((void*)((uint64_t)index << 21)) : "memory");
    return 1;
}