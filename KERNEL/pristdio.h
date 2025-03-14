#ifndef _PRIMARY_STDIO_H_
#define _PRIMARY_STDIO_H_

#include <video.h>
#include <stdint.h>

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
	iouthex32((i>32)&0xffffffff);
	iouthex32(i&0xffffffff);
}
void Dump256(int8_t* sour) {
	for (int i=0;i<16;i++) {
		iouthex32((uint32_t)sour+i*16);
		outstr("|");
		for (int j=0;j<16;j++) {
			iouthex8(sour[i*16+j]);
			outstr(" ");
		}
		outstr("\b|");
		for (int j=0;j<16;j++) {
			drawfont(sour[i*16+j]);
			CursorX++;
		}
		outstr("\n");
	}
}

#endif
