#include <printk.h>

extern video_t video;
char ch_buffer[1024];

int printk(const char* format, ...) {
	uint64_t flags;
    asm volatile(
        "pushfq; pop %0; cli" 
        : "=r"(flags) 
        : 
        : "memory"
    );

	va_list args;
	va_start(args, format);
	int len = vsprintf(ch_buffer, format, args);
	va_end(args);
	for (int i = 0; i < len; i++) {
		putchar(&video, ch_buffer[i]);
	}

	asm volatile(
        "push %0; popfq" 
        : 
        : "r"(flags) 
        : "memory"
    );

	return len;
}