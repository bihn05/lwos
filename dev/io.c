#include "stdint.h"
#include "io.h"

BYTE inb(WORD port) {
    BYTE val;
    __asm__ volatile ("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}
void outb(WORD port, BYTE val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

WORD inw(WORD port) {
    WORD val;
    __asm__ volatile ("inw %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}
void outw(WORD port, WORD val) {
    __asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

DWORD inl(WORD port) {
    DWORD val;
    __asm__ volatile ("inl %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}
void outl(WORD port, DWORD val) {
    __asm__ volatile ("outl %0, %1" : : "a"(val), "Nd"(port));
}