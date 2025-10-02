#ifndef STRING_H
#define STRING_H

#include <stdint.h>

void* memcpy(void* dest, const void* src, size_t n) {
	uint8_t* d = (uint8_t*)dest;
	const uint8_t* s = (const unsigned char*)src;
	for (size_t i = 0; i < n; i++) {
		d[i] = s[i];
	}
	return dest;
}
void memset(void* s, uint8_t c, size_t n) {
	uint8_t* p = (uint8_t*)s;
	for (size_t i = 0; i < n; i++) {
		p[i] = c;
	}
}
size_t strlen(const char* str) {
	size_t len = 0;
	while (str[len] != '\0') {
		len++;
	}
	return len;
}

#endif