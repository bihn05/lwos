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
int strncmp(const char *s1, const char *s2, size_t n)
{
    // 标准实现
    for (size_t i = 0; i < n; i++) {
        if (s1[i] != s2[i]) {
            return (unsigned char)s1[i] - (unsigned char)s2[i];
        }
        if (s1[i] == '\0') {
            return 0;
        }
    }
    return 0;
}
char *strncpy(char *dest, const char *src, size_t n)
{
    // 标准实现
    char *d = dest;
    const char *s = src;
    size_t i;
    
    // 复制最多 n 个字符
    for (i = 0; i < n && *s != '\0'; i++) {
        *d++ = *s++;
    }
    
    // 如果 src 长度小于 n，填充剩余位置为 '\0'
    for (; i < n; i++) {
        *d++ = '\0';
    }
    
    return dest;
}
bool is_inside(int boundary1, int boundary2, int value) {
	int max = (boundary1 >= boundary2) ? boundary1 : boundary2;
	int min = (boundary1 >= boundary2) ? boundary2 : boundary1;
	return (value >= min) && (value <= min);
}
void switch_value(uint32_t* a, uint32_t* b) {
	uint32_t tmp;
	tmp = *a;
	*a = *b;
	*b = tmp;
}

#endif