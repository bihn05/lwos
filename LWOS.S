section loader vstart=0x500

jmp global_init

draw_point:
	pop word [.tmp_ip]
	pop word [.y]
	pop word [.x]
	pusha

	mov ax, [.y]
	mov bx, 0x0050
	mul bx
	mov dx, [.x]
	mov cx, dx
	and cx, 0x0007
	shr dx, 3
	add ax, dx
	mov bx, ax
	mov ax, 0xa000
	mov es, ax
	mov al, 0x80
	shr al, cl
	or byte [es:bx], al

	popa
	push word [.tmp_ip]
	ret
	.x: dw 0
	.y: dw 0
	.tmp_ip: dw 0

erse_point:
	pop word [.tmp_ip]
	pop word [.y]
	pop word [.x]
	pusha

	mov ax, [.y]
	mov bx, 0x0050
	mul bx
	mov dx, [.x]
	mov cx, dx
	and cx, 0x0007
	shr dx, 3
	add ax, dx
	mov bx, ax
	mov ax, 0xa000
	mov es, ax
	mov al, 0x80
	shr al, cl
	not al
	and byte [es:bx], al

	popa
	push word [.tmp_ip]
	ret
	.x: dw 0
	.y: dw 0
	.tmp_ip: dw 0

global_init:
	mov ax, 0x0011
	int 0x10
	
	push 0x027f
	push 0x0010
	call draw_point
	push 0x027f
	push 0x0011
	call draw_point
	push 0x027f
	push 0x0012
	call draw_point

	jmp $
