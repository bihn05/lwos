#ifndef _GRAPHICS_H
#define _GRAPHICS_H

#include <stdint.h>
#include <driver/pci.h>
#include <driver/graphic/vbe.h>
#include <mm/mmio.h>
#include <print.h>

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t bpp;
    uint32_t pitch;     // 先可暂时按 width * (bpp/8)
    uint64_t fb_phys;
    void *fb_virt;
} bga_fb_t;

extern bga_fb_t g_fb;
void bga_graphic_init();
void putpixel(uint32_t x, uint32_t y, uint32_t color);

typedef struct video_device video_device_t;

typedef struct {
    bool (*set_mode)(video_device_t *dev, uint32_t w, uint32_t h, uint32_t bpp);
    void (*putpixel)(video_device_t *dev, uint32_t x, uint32_t y, uint32_t color);
    void (*fill)(video_device_t *dev, uint32_t color);
} video_ops_t;

struct video_device {
    const char *name;

    uint32_t width;
    uint32_t height;
    uint32_t bpp;
    uint32_t pitch;

    uint64_t fb_phys;
    void *fb_virt;
    uint64_t fb_size;

    void *priv;              // 指向具体硬件私有数据
    const video_ops_t *ops;
};

typedef struct {
    pci_addr_t pci;
    uint16_t vbe_id;
} bochs_vbe_priv_t;

bool bochs_vbe_init(video_device_t *dev);

typedef struct gfx_surface gfx_surface_t;

typedef struct {
    void (*putpixel)(gfx_surface_t *surf, uint32_t x, uint32_t y, uint32_t color);
    void (*draw_rect)(gfx_surface_t *surf, bool fill, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
    void (*draw_char)(gfx_surface_t *surf, uint32_t x, uint32_t y, char c, uint32_t fg, uint32_t bg);
    void (*fill)(gfx_surface_t *surf, uint32_t color);
} gfx_ops_t;

typedef struct gfx_surface {
    uint32_t width;
    uint32_t height;
    uint32_t bpp;
    uint32_t pitch;

    void *pixels;              // 实际像素内存
    video_device_t *device;    // 可选：指回底层设备
    const gfx_ops_t *ops;
} gfx_surface_t;

void gfx_surface_from_device(gfx_surface_t *surf, video_device_t *dev);

#endif