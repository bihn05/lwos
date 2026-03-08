#include <task/elf.h>
#include <string.h>

#define PT_LOAD 1
#define PF_X 1
#define PF_W 2
#define PF_R 4

static uint64_t elf_flags_to_pte(uint32_t pf) {
    uint64_t flags = PTE_US;
    if (pf & PF_W) flags |= PTE_RW;
    return flags;
}

uint64_t load_elf(uint64_t pml4_pa, vfs_node_t* elf_node) {
    if (!elf_node) return 0;

    uint8_t* file_buf = (uint8_t*)kmalloc(elf_node->size);
    if (!file_buf) return 0;

    elf_node->ops->read(elf_node, 0, elf_node->size, file_buf);

    Elf64_Ehdr* ehdr = (Elf64_Ehdr*)file_buf;
    if (*(uint32_t*)ehdr->e_ident != 0x464c457f) {
        kfree(file_buf);
        return 0;
    }

    Elf64_Phdr* phdrs = (Elf64_Phdr*)(file_buf + ehdr->e_phoff);

    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdrs[i].p_type != PT_LOAD) continue;

        uint64_t seg_start = phdrs[i].p_vaddr & PAGE_MASK;
        uint64_t seg_end   = (phdrs[i].p_vaddr + phdrs[i].p_memsz + PAGE_SIZE - 1) & PAGE_MASK;
        uint64_t pte_flags = elf_flags_to_pte(phdrs[i].p_flags);

        for (uint64_t va = seg_start; va < seg_end; va += PAGE_SIZE) {
            uint64_t pa = pmm_alloc_page();
            if (!pa) {
                kfree(file_buf);
                return 0;
            }

            memset(pa_to_ptr(pa), 0, PAGE_SIZE);

            if (!vmm_map_page(pml4_pa, va, pa, pte_flags)) {
                kfree(file_buf);
                return 0;
            }
        }

        // 把段内容按页写入已分配物理页
        for (uint64_t off = 0; off < phdrs[i].p_filesz; off++) {
            uint64_t va = phdrs[i].p_vaddr + off;

            uint64_t* pte = pt_get_pte(pml4_pa, va, 0, 0);
            if (!pte || !(*pte & PTE_P)) {
                kfree(file_buf);
                return 0;
            }

            uint64_t pa = entry_addr(*pte) + (va & 0xFFF);
            *(uint8_t*)pa_to_ptr(pa) = file_buf[phdrs[i].p_offset + off];
        }
    }

    kfree(file_buf);
    return ehdr->e_entry;
}