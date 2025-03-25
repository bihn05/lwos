[bits 32]

_interrupt_handler:
	add esp, 4
	xchg bx, bx
	iret
global _interrupt_handler