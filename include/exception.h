#ifndef _EXCEPTION_C
#define _EXCEPTION_C

#include <stdint.h>
#include <interrupt.h>

void dump_registers(int_registers_t* regs);
void page_fault_handler(int_registers_t* regs);

#endif