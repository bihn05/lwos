#include <graphics.h>
#include <mm.h>

extern uint64_t kernel_pm4;
bga_fb_t g_fb;

static bochs_vbe_priv_t g_bochs_priv;
static video_device_t g_bochs_dev;

void bga_graphic_init() {
    vbe_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
    vbe_write(VBE_DISPI_INDEX_XRES, 1024);
    vbe_write(VBE_DISPI_INDEX_YRES, 768);
    vbe_write(VBE_DISPI_INDEX_BPP, 32);
    vbe_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED);

    void *fb_virt = mmio_map_region(kernel_pm4, 0xE0000000ULL, (16 * 1024 * 1024ULL), PTE_PCD | PTE_PWT);
    volatile uint32_t *fb = (volatile uint32_t *)fb_virt;
    g_fb.fb_phys = 0xE0000000ULL;
    g_fb.fb_virt = fb_virt;
    g_fb.width = 1024;
    g_fb.height = 768;
    g_fb.bpp = 32;
    g_fb.pitch = g_fb.width * (g_fb.bpp / 8);
}
void putpixel(uint32_t x, uint32_t y, uint32_t color) {
    if (!g_fb.fb_virt || g_fb.bpp != 32) return;

    volatile uint32_t *fb = (volatile uint32_t *)g_fb.fb_virt;
    uint64_t offset = (uint64_t)y * g_fb.pitch + (uint64_t)x * (g_fb.bpp / 8);
    fb[offset / 4] = color; // 每像素 4 字节
}
static bool bochs_vbe_set_mode(video_device_t *dev, uint32_t w, uint32_t h, uint32_t bpp) {
    // 写 0x1CE/0x1CF
    // 更新 dev->width / height / bpp / pitch
    // 如有必要重新映射 fb
    dev->width = w;
    dev->height = h;
    dev->bpp = bpp;
    dev->pitch = w * (bpp / 8);
    // 重新映射如有需要
    return true;
}

static void bochs_vbe_putpixel(video_device_t *dev, uint32_t x, uint32_t y, uint32_t color) {
    if (x >= dev->width || y >= dev->height) return;
    if (!dev->fb_virt || dev->bpp != 32) return;

    volatile uint32_t *fb = (volatile uint32_t *)dev->fb_virt;
    uint32_t stride = dev->pitch / 4;
    fb[y * stride + x] = color;
}

static void bochs_vbe_fill(video_device_t *dev, uint32_t color) {
    if (!dev->fb_virt || dev->bpp != 32) return;

    volatile uint32_t *fb = (volatile uint32_t *)dev->fb_virt;
    uint64_t pixels = (uint64_t)(dev->pitch / 4) * dev->height;
    for (uint64_t i = 0; i < pixels; i++) {
        fb[i] = color;
    }
}

static const video_ops_t bochs_vbe_ops = {
    .set_mode = bochs_vbe_set_mode,
    .putpixel = bochs_vbe_putpixel,
    .fill     = bochs_vbe_fill,
};

bool bochs_vbe_init(video_device_t *dev) {
    memset(dev, 0, sizeof(*dev));

    dev->name = "bochs-vbe";
    dev->priv = &g_bochs_priv;
    dev->ops  = &bochs_vbe_ops;

    uint32_t w = 1024, h = 768, bpp = 32;

    // 这里做:
    // 1. 检测 VBE ID
    // 2. 设置默认模式
    // 3. 映射 framebuffer
    // 4. 填 width/height/bpp/pitch/fb_phys/fb_virt/fb_size
    vbe_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
    vbe_write(VBE_DISPI_INDEX_XRES, w);
    vbe_write(VBE_DISPI_INDEX_YRES, h);
    vbe_write(VBE_DISPI_INDEX_BPP, 32);
    vbe_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED);

    void *fb_virt = mmio_map_region(kernel_pm4, 0xE0000000ULL, (16 * 1024 * 1024ULL), PTE_PCD | PTE_PWT);
    volatile uint32_t *fb = (volatile uint32_t *)fb_virt;

    dev->width = w;
    dev->height = h;
    dev->bpp = 32;
    dev->pitch = dev->width * (dev->bpp / 8);
    dev->fb_phys = 0xE0000000ULL;
    dev->fb_virt = fb_virt;
    dev->fb_size = (uint64_t)dev->pitch * dev->height;

    return true;
}

void gfx_putpixel(gfx_surface_t *surf, uint32_t x, uint32_t y, uint32_t color) {
    if (surf->device && surf->device->ops && surf->device->ops->putpixel) {
        surf->device->ops->putpixel(surf->device, x, y, color);
    }
}
void gfx_fill(gfx_surface_t *surf, uint32_t color) {
    if (surf->device && surf->device->ops && surf->device->ops->fill) {
        surf->device->ops->fill(surf->device, color);
    }
}
void gfx_draw_rect(gfx_surface_t *surf, bool fill, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    if (fill) {
        for (uint32_t j = 0; j < h; j++) {
            for (uint32_t i = 0; i < w; i++) {
                gfx_putpixel(surf, x + i, y + j, color);
            }
        }
    } else {
        for (uint32_t i = 0; i < w; i++) {
            gfx_putpixel(surf, x + i, y, color);
            gfx_putpixel(surf, x + i, y + h - 1, color);
        }
        for (uint32_t j = 0; j < h; j++) {
            gfx_putpixel(surf, x, y + j, color);
            gfx_putpixel(surf, x + w - 1, y + j, color);
        }
    }
}
void gfx_draw_char(gfx_surface_t *surf, uint32_t x, uint32_t y, char c, uint32_t fg, uint32_t bg) {
    gfx_draw_rect(surf, true, x, y, 8, 16, bg);
    for (int j=0;j<16;j++) {
        for (int i=0;i<8;i++) {
            if (g_8x16_font[(uint8_t)c*16 + j] & (1 << (7 - i))) {
                gfx_putpixel(surf, x + i, y + j, fg);
            }
        }
    }
}

static const gfx_ops_t gfx_draw_ops = {
    .putpixel = gfx_putpixel,
    .draw_rect = gfx_draw_rect,
    .draw_char = gfx_draw_char,
    .fill = gfx_fill,
};

void gfx_surface_from_device(gfx_surface_t *surf, video_device_t *dev) {
    surf->width  = dev->width;
    surf->height = dev->height;
    surf->bpp    = dev->bpp;
    surf->pitch  = dev->pitch;
    surf->pixels = dev->fb_virt;
    surf->device = dev;
    surf->ops  = &gfx_draw_ops;
}