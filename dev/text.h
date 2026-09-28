#ifndef _LW_TEXT_H
#define _LW_TEXT_H

#include "stdint.h"

// out
void set_cursor(void);
void get_cursor(void);
void screen_scroll(void);
void screen_clear(void);
void putc(char c); // put char
void putca(char c); // put char allowed
void puts(char* s); // put string (until *s==0)
void put_byte(BYTE v);
void put_word(WORD v);
void put_dword(DWORD v);
void put_qword(QWORD v);
void dump128(PVOID src);

// in
int getk(void);
char getc(void);

#endif