[bits 32]

extern kernel_init
global start
start:
	call kernel_init
	jmp $