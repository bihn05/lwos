#ifndef _VSPRINTF_H
#define _VSPRINTF_H

#include <stdarg.h>
#include <string.h>
#include <stdint.h>

#define ZEROPAD		0x01
#define SIGN		0x02
#define PLUS		0x04
#define SPACE		0x08
#define LEFT		0x10
#define SPECIAL		0x20
#define SMALL		0x40
#define DOUBLE		0x80

#define is_digit(c) ((c) >= '0' && (c) <= '9')

static int skip_atoi(const char **s);
static char* number(char* str, uint64_t* num, int base, int size, int precision, int flags);
int vsprintf(char* buf, const char* fmt, va_list args);
int sprintf(char *buf, const char *fmt, ...);
int snprintf(char *buf, int size, const char *fmt, ...);

#endif