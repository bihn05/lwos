// graphic vbe

#ifndef _VBE_H
#define _VBE_H

#include <stdint.h>
#include <driver/port.h>

static inline void bga_write(uint16_t index, uint16_t value);
static inline uint16_t bga_read(uint16_t index);

void vbe_write(uint16_t index, uint16_t value);
uint16_t vbe_read(uint16_t index);
bool bga_detect_and_map(void);
bool bga_set_mode(uint64_t pm4_pa, uint32_t width, uint32_t height, uint32_t bpp);
void bga_fill(uint32_t color);

#endif