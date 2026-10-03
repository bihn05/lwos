#include "mem.h"
#include "stdint.h"

PVOID memcpy(PVOID dst, PCVOID src, int count) {
    void * ret = dst;
    while (count--) {
        *(char *)dst = *(char *)src;
        dst = (char *)dst + 1;
        src = (char *)src + 1;
    }
    return ret;
}
PVOID memzero(PVOID dst, int len) {
    PBYTE p = (PBYTE)dst;
    while (len-- > 0) {
        *(p++)=0;
    }
    return dst;
}