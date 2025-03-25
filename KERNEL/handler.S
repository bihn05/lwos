[bits 32]

section .text
extern _printk

global _interrupt_handler
_interrupt_handler:
	xchg bx, bx

	push message
	call _printk
	add esp, 4

	xchg bx, bx
	iret

section .data

message:
	db "Default Interrupt", 0xa, 0