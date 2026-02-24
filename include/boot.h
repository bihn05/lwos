#ifndef _BOOT_H
#define _BOOT_H

#include <stdint.h>

typedef struct {
    uint64_t magic;
    uint64_t framebuffer_ptr;
} boot_info_t;

void kernel_init();

#endif