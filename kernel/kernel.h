#ifndef _KERNEL_H
#define _KERNEL_H

#include <stdint.h>
#include <stdarg.h>
#include <driver/video.h>
#include <vsprintf.h>

static char ch_buffer[1024];;
int printk(const char* format, ...) {
	va_list args;
	va_start(args, format);
	int len = vsprintf(ch_buffer, format, args);
	va_end(args);
	for (int i = 0; i < len; i++) {
		putchar(ch_buffer[i]);
	}
	return len;
}

#endif