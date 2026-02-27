#include <task/elf.h>
#include <string.h>

uint64_t load_elf(vfs_node_t* elf_node) {
    if (elf_node == NULL) {
        printk("ELF load failed: elf_node is NULL\n");
        return -1;
    }

    uint8_t* file_buf = (uint8_t*)kmalloc(elf_node->size);
    elf_node->ops->read(elf_node, 0, elf_node->size, file_buf);

    Elf64_Ehdr* ehdr = (Elf64_Ehdr*)file_buf;
    uint32_t* magic = (uint32_t*)ehdr->e_ident;
    if (*magic != 0x464c457f) { // \x7F ELF
        printk("ELF load failed: Invalid magic number\n");
        return -1;
    }

    if (ehdr->e_machine != 62 || ehdr->e_type != 2) { // EM_X86_64 = 62, ET_EXEC = 2
        printk("Unsupported ELF type or machine: type=%d, machine=%d\n", ehdr->e_type, ehdr->e_machine);
        kfree(file_buf);
        return -1;
    }

//  printk("ELF entry point: 0x%08X%08X\n", (uint32_t)(ehdr->e_entry >> 32), (uint32_t)(ehdr->e_entry & 0xFFFFFFFF));

    Elf64_Phdr* phdrs = (Elf64_Phdr*)(file_buf + ehdr->e_phoff);
    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdrs[i].p_type == PT_LOAD) {
            printk("Loading segmant %d: Vaddr 0x%08x%08x, Size %d bytes\n",
                i, (uint32_t)(phdrs[i].p_vaddr >> 32), (uint32_t)(phdrs[i].p_vaddr & 0xFFFFFFFF), 
                (uint32_t)phdrs[i].p_memsz);
            
            vmm_alloc_map_region(phdrs[i].p_vaddr, phdrs[i].p_memsz, PAGE_PRESENT | PAGE_RW);
            uint8_t* dest = (uint8_t*)(phdrs[i].p_vaddr);

            memcpy(dest, file_buf + phdrs[i].p_offset, phdrs[i].p_filesz);
            if (phdrs[i].p_memsz > phdrs[i].p_filesz) {
                memset(dest + phdrs[i].p_filesz, 0, phdrs[i].p_memsz - phdrs[i].p_filesz);
            }
        }
    }

    return ehdr->e_entry;
}