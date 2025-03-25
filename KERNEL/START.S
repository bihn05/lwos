[bits 32]

extern _kernel_init
global _start
_start:
	xchg bx, bx
	call _kernel_init
	xchg bx, bx
	jmp $
