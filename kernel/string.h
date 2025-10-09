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
char* strcpy(char* dest, const char* src) {
	if (dest == NULL || src == NULL) {
		return 0;
	}
	char* ret = dest;
	while ((*dest++ = *src++) != '\0');
	return ret;
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
int strcmp(const char* cs, const char* ct) {
	uint8_t c1, c2;
	while (1) {
		c1 = *cs++;
		c2 = *ct++;
		if (c1 != c2) {
			return c1 < c2 ? -1 : 1;
		}
		if (!c1) {
			break;
		}
	}
	return 0;
}

#endif