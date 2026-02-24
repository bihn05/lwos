// kernel/main.c

#include <print.h>
#include <descript.h>

video_t video;

void kernel_init() {
    video.frame_buf = (uint8_t*)0xa0000;
    writereg_video(g_640x480x2);
    video_clear(&video);
    gdt_tss_init();
    putchar(&video, 'S');

    while (1) {
        asm volatile ("hlt");
    }
}