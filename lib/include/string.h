#ifndef _STRING_H
#define _STRING_H

#include "stdint.h"

const char *strtok(const char *src, char dec);
int strcmp(const char *s1, const char *s2);
unsigned int strlen(const char *s);
char *strcpy(char *dest, const char *src);
void skip_ws(const char **p);

#endif