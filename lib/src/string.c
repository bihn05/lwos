#include "stdint.h"

const char *strtok(const char *src, char dec) {
    char *p=src;
    while (*p++) {
        if (*p==dec) {
            *p=0;
            return p+1;
        }
    }
    return p;
}
int strcmp(const char *s1, const char *s2) {
	while (*s1 == *s2++)
		if (*s1++ == 0)
			return (0);
	return (*(PCBYTE)s1 - *(PCBYTE)--s2);
}
unsigned int strlen(const char *s) {
    const char *p = s;
    while (*p)
        p++;
    return (unsigned int)(p - s);
}
char *strcpy(char *dest, const char *src) {
    char *ret = dest;
    while ((*dest++ = *src++))
        ;
    return ret;
}
void skip_ws(const char **p) {
    while (**p==' '||**p=='\t') {
        (*p)++;
    }
}