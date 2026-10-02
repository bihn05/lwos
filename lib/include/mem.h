#ifndef _MEM_H
#define _MEM_H

#include "stdint.h"

PVOID memcpy(PVOID dst, PVOID src, int count);
PVOID memzero(PVOID dst, int len);

#endif