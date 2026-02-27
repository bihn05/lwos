#ifndef _DESCRIPT_H
#define _DESCRIPT_H

#include <stdint.h>

#define GDT_ENTRIES 7 // 0:NULL, 1:K_CS, 2:K_DS, 3:U_DS, 4:U_CS, 5-6:TSS (16字节)

#pragma pack(push, 1)

typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} gdt_entry_t; // gdt

typedef struct {
    gdt_entry_t low;
    uint32_t    base_upper;
    uint32_t    reserved;
} gdt_tss_entry_t; // tss

typedef struct {
    uint16_t limit;
    uint64_t base;
} gdtr_t; // GDT register

typedef struct {
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist[7]; // interrupt stack table
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap_base;
} tss_t;

#pragma pack(pop)

// 全局变量
static gdt_entry_t gdt[GDT_ENTRIES];
static tss_t       kernel_tss;

extern void load_segments();
void gdt_set_gate(int num, uint8_t access, uint8_t gran);
void gdt_set_tss(int num, uint64_t base, uint32_t limit);
void gdt_tss_init();

#endif