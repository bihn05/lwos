#ifndef __STRING_H_
#define __STRING_H_

#include <stdint.h>

void* memcpy(void* dest, const void* src, size_t n);
char* strcpy(char* dest, const char* src);
void memset(void* s, uint8_t c, size_t n);
size_t strlen(const char* str);
int strcmp(const char* cs, const char* ct);
int strncmp(const char *s1, const char *s2, size_t n);
char* strncpy(char* dest, const char* src, size_t n);
char* strcat(char* dest, const char* src);
char* strncat(char* dest, const char* src, size_t n);

bool is_inside(int boundary1, int boundary2, int value);
void switch_value(uint32_t* a, uint32_t* b);

#endif