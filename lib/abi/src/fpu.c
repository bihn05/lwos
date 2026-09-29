#include "fpu.h"

void fpu_init(void) {
    DWORD cr0;
    asm volatile ("mov %%cr0, %0" : "=r"(cr0));
    cr0&=~(1u<<2);
    cr0&=~(1u<<3);
    cr0|=(1u<<1);
    cr0|=(1u<<5);
    asm volatile ("mov %0, %%cr0" :: "r"(cr0));
    asm volatile ("fninit");
}