// kernel/fsys/elf.h

#ifndef _ELF_H_
#define _ELF_H_

#include <stdint.h>
#include <fsys/vfs.h>
#include <mm/km.h>
#include <string.h>
#include <kernel.h>

#define ELF_MAGIC 0x464C457F // "\x7FELF" 的小端序表示

// ELF 文件头
typedef struct {
    uint8_t  e_ident[16];   // 魔数和识别信息
    uint16_t e_type;        // 文件类型 (1=Relocatable, 2=Executable)
    uint16_t e_machine;     // 架构 (3=x86)
    uint32_t e_version;     // 版本
    uint32_t e_entry;       // 程序的入口点虚拟地址 (Entry Point)
    uint32_t e_phoff;       // 程序头表 (Program Header Table) 的偏移
    uint32_t e_shoff;       // 节头表 (Section Header Table) 的偏移
    uint32_t e_flags;
    uint16_t e_ehsize;      // ELF 文件头的大小
    uint16_t e_phentsize;   // 单个程序头的大小
    uint16_t e_phnum;       // 程序头的数量
    uint16_t e_shentsize;   // 单个节头的大小
    uint16_t e_shnum;       // 节头的数量
    uint16_t e_shstrndx;
} Elf32_Ehdr;

// ELF 程序头 (描述如何将文件映射到内存)
#define PT_LOAD 1 // 表示这是一个需要被加载到内存的段

typedef struct {
    uint32_t p_type;        // 段类型 (如 PT_LOAD)
    uint32_t p_offset;      // 该段在文件中的偏移
    uint32_t p_vaddr;       // 该段在内存中的虚拟地址
    uint32_t p_paddr;       // 物理地址 (通常等同于 p_vaddr)
    uint32_t p_filesz;      // 该段在文件中的大小
    uint32_t p_memsz;       // 该段在内存中的大小 (可能大于 filesz，比如 .bss 段会被清零)
    uint32_t p_flags;       // 读/写/执行权限
    uint32_t p_align;       // 对齐要求
} Elf32_Phdr;

// return an entry point on success, or -1 on failure
uint32_t load_elf(vfs_node_t* elf_node) {
    if (elf_node == NULL) {
        printk("ELF node is NULL\n");
        return -1;
    }

    uint8_t* file_buf = (uint8_t*)kmalloc(elf_node->size);
    elf_node->ops->read(elf_node, 0, elf_node->size, file_buf);

    Elf32_Ehdr* ehdr = (Elf32_Ehdr*)file_buf;

    uint32_t* magic = (uint32_t*)ehdr->e_ident;
    if (*magic != ELF_MAGIC) {
        printk("Invalid ELF magic: 0x%08X\n", *magic);
        kfree(file_buf);
        return -1;
    }

    if (ehdr->e_machine != 3 || ehdr->e_type != 2) {
        printk("Unsupported ELF type or machine: type=%d, machine=%d\n", ehdr->e_type, ehdr->e_machine);
        kfree(file_buf);
        return -1;
    }

    printk("ELF entry point: 0x%08X\n", ehdr->e_entry);

    // program header table
    Elf32_Phdr* phdrs = (Elf32_Phdr*)(file_buf + ehdr->e_phoff);
    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdrs[i].p_type == PT_LOAD) {
            printk("Loading segment %d: Vaddr 0x%x, Size %d bytes\n", i, phdrs[i].p_vaddr, phdrs[i].p_memsz);

            vmm_alloc_map_region(phdrs[i].p_vaddr, phdrs[i].p_memsz, PAGE_PRESENT | PAGE_RW | PAGE_USER);
            uint8_t* dest = (uint8_t*)phdrs[i].p_vaddr;

            memcpy(dest, file_buf + phdrs[i].p_offset, phdrs[i].p_filesz);

            if (phdrs[i].p_memsz > phdrs[i].p_filesz) {
                memset(dest + phdrs[i].p_filesz, 0, phdrs[i].p_memsz - phdrs[i].p_filesz);
            }
        }
    }

    return ehdr->e_entry;
}

#endif