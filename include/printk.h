#ifndef _PRINTK_H
#define _PRINTK_H

#include <stdarg.h>
#include <print.h>
#include <vsprintf.h>

extern char ch_buffer[1024];
int printk(const char* format, ...);

#endif