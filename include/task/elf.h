#ifndef _ELF_H
#define _ELF_H

#include <stdint.h>
#include <fsys/vfs.h>
#include <mm.h>
#include <printk.h>

// elf64 header
typedef struct {
    unsigned char e_ident[16]; 
    uint16_t      e_type;      
    uint16_t      e_machine;
    uint32_t      e_version;   
    uint64_t      e_entry;     // 64 位入口点
    uint64_t      e_phoff;     // 64 位偏移
    uint64_t      e_shoff;     
    uint32_t      e_flags;     
    uint16_t      e_ehsize;    
    uint16_t      e_phentsize; 
    uint16_t      e_phnum;     
    uint16_t      e_shentsize; 
    uint16_t      e_shnum;     
    uint16_t      e_shstrndx;  
} Elf64_Ehdr;

// elf64 program header
typedef struct {
    uint32_t p_type;    
    uint32_t p_flags;
    uint64_t p_offset;  
    uint64_t p_vaddr;   
    uint64_t p_paddr;   
    uint64_t p_filesz;  
    uint64_t p_memsz;   
    uint64_t p_align;   
} Elf64_Phdr;

#define PT_LOAD 1

uint64_t load_elf(vfs_node_t* elf_node);

#endif