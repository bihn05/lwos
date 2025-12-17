#ifndef _DEBUG_H
#define _DEBUG_H

void bxbp(void) {
	__asm volatile("xchg %bx, %bx");
}
int cheax(void) {
	return 0xcafebabe;
}

#endif