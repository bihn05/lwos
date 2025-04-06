[bits 32]

extern _kernel_init
global _start
_start:
	call _kernel_init
	xchg bx, bx
	int 0x1f
	jmp $
