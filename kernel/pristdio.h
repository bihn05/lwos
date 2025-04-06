#ifndef _PRIMARY_STDIO_H_
#define _PRIMARY_STDIO_H_

#include <video.h>
#include <type.h>
#include <stdarg.h>

const char _STRHEX[17]="0123456789ABCDEF";
void iouthex4(uint8_t i) {
	putchar(_STRHEX[i&0xf]);
}
void iouthex8(uint8_t i) {
	putchar(_STRHEX[i>>4&0xf]);
	putchar(_STRHEX[i&0xf]);
}
void iouthex16(uint16_t i) {
	iouthex8((i>>8)&0xff);
	iouthex8(i&0xff);
}
void iouthex32(uint32_t i) {
	iouthex16((i>>16)&0xffff);
	iouthex16(i&0xffff);
}
void iouthex64(uint64_t i) {
	iouthex32((i>>32)&0xffffffff);
	iouthex32(i&0xffffffff);
}
void Dump256(uint8_t* sour) {
	for (int i=0;i<16;i++) {
		iouthex32((uint32_t)sour+i*16);
		outstr("|");
		for (int j=0;j<16;j++) {
			iouthex8(sour[i*16+j]);
			if (j == 7) {
				outstr("`");
			}
			else {
				outstr(" ");
			}
		}
		outstr("\b|");
		for (int j=0;j<16;j++) {
			drawfont(sour[i * 16 + j]);
			CursorX++;
		}
		outstr("\n");
	}
}
static char buf[1024];
int printk(const char* fmt, ...) {
	va_list args;
	int i;

	va_start(args, fmt);

	i = vsprintf(buf, fmt, args);

	va_end(args);
	for (int k = 0; k < i; k++) {
		putchar(buf[k]);
	}
}

#endif
