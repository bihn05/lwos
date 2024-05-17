section loader vstart=0x500

jmp global_init

global_init:
	mov ax, 0xb800
	mov es, ax
	mov bx, 0
	mov byte [es:bx], 'K'

	jmp $