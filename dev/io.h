#ifndef _LW_IO_H
#define _LW_IO_H

#include "stdint.h"

BYTE inb(WORD port);
WORD inw(WORD port);
DWORD inl(WORD port);
void outb(WORD port, BYTE val);
void outw(WORD port, WORD val);
void outl(WORD port, DWORD val);

static inline void io_wait(void)
{
	outb(0x80, 0);
}

#endif