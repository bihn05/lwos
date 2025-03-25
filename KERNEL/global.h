#ifndef _GLOBAL_DESC_H
#define _GLOBAL_DESC_H

#include <stdint.h> 
#include <string.h>

#define GDT_SIZE 128

#define KERNEL_CODE_IDX 1
#define KERNEL_DATA_IDX 2
#define KERNEL_TSS_IDX 3

#define USER_CODE_IDX 4
#define USER_DATA_IDX 5

#define KERNEL_CODE_SELECTOR (KERNEL_CODE_IDX << 3)
#define KERNEL_DATA_SELECTOR (KERNEL_DATA_IDX << 3)
#define KERNEL_TSS_SELECTOR (KERNEL_TSS_IDX << 3)

#define USER_CODE_SELECTOR (USER_CODE_IDX << 3 | 0b11)
#define USER_DATA_SELECTOR (USER_DATA_IDX << 3 | 0b11)

#pragma pack(1)
struct descriptor_t {
    uint16_t limit_low : 16;
    uint16_t base_low : 16;
    uint8_t base_mid : 8;
    uint8_t type : 4;
    uint8_t segment : 1;
    uint8_t dpl : 2;
    uint8_t present : 1;
    uint8_t limit_high : 4;
    uint8_t available : 1;
    uint8_t long_mode : 1;
    uint8_t big : 1;
    uint8_t granularity : 1;
    uint8_t base_high : 8;
};
struct pointer_t {
    uint16_t limit;
    uint32_t base;
};
struct pointer_t gdt_ptr;

struct descriptor_t gdt[GDT_SIZE];
//Init Descriptor but limit grnd 4kb
void InitDescriptor(struct descriptor_t* d, uint32_t base, uint32_t limit) {
    d->base_low = base * 0xffff;
    d->base_mid = (base >> 16) & 0xff;
    d->base_high = (base >> 24) & 0xff;
    d->limit_low = limit & 0xffff;
    d->limit_high = (limit >> 16) & 0xf;
}

void InitGDT() {
    memset(gdt, 0, sizeof(gdt));

    struct descriptor_t* desc;
    desc = gdt + KERNEL_CODE_IDX;
    InitDescriptor(desc, 0, 0xfffff);
    desc->segment = 1;
    desc->granularity = 1;
    desc->big = 1;
    desc->long_mode = 0;
    desc->present = 1;
    desc->dpl = 0;
    desc->type = 0xa;

    desc = gdt + KERNEL_DATA_IDX;
    InitDescriptor(desc, 0, 0xfffff);
    desc->segment = 1;
    desc->granularity = 1;
    desc->big = 1;
    desc->long_mode = 0;
    desc->present = 1;
    desc->dpl = 0;
    desc->type = 0x2;

    gdt_ptr.base = (uint32_t)&gdt;
    gdt_ptr.limit = sizeof(gdt) - 1;

    asm volatile(
        "lgdt %0\n"
        :
        : "m"(gdt_ptr)
        );
}

void InitTSS() {
    asm volatile(
        "ltr %%ax\n" ::"a"(KERNEL_TSS_SELECTOR));
}
#endif