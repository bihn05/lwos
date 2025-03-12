section loader vstart=0x700

jmp global_init



; set_cursor(point);
set_cursor:
	push bp
	mov bp, sp

	mov bx, [bp+4]

	mov dx, 0x3d4
	mov al, 0x0f
	out dx, al
	mov dx, 0x3d5
	mov al, bl
	out dx, al
	mov dx, 0x3d4
	mov al, 0x0e
	out dx, al
	mov dx, 0x3d5
	mov al, bh
	out dx, al

	pop bp
	ret

; get_cursor(*point)
get_cursor:
	push bp
	mov bp, sp
	
	mov dx, 0x3d4
	mov al, 0x0e
	out dx, al
	mov dx, 0x3d5
	in al, dx
	shl ax, 8
	mov dx, 0x3d4
	mov al, 0x0f
	out dx, al
	mov dx, 0x3d5
	in al, dx

	mov bx, [bp+4]
	mov [bx], ax

	pop bp
	ret

; putchara(uint_16 char_attr)
putchara:
	push bp
	mov bp, sp
	sub sp, 4
	lea ax, [bp-2]
	mov bx, sp
	mov [bx], ax
	call get_cursor
	mov bx, [bp-2]
	shl bx, 1
	mov ax, [bp+4]
	mov [es:bx], ax
	shr bx, 1
	inc bx
	mov ax, bx
	mov bx, sp
	mov [bx], ax
	call set_cursor
	add sp, 4

	pop bp
	ret

; putchar(uint_16 ch)
putchar:
	push bp
	mov bp, sp
	sub sp, 4
	lea ax, [bp-2]
	mov bx, sp
	mov [bx], ax
	call get_cursor
	mov bx, [bp-2]
	shl bx, 1
	mov ax, [bp+4]
	mov [es:bx], al
	shr bx, 1
	inc bx
	mov ax, bx
	mov bx, sp
	mov [bx], ax
	call set_cursor
	add sp, 4

	pop bp
	ret

; nxtl()
nxtl:
	push bp
	mov bp, sp
	sub sp, 4
	lea ax, [bp-2]
	mov bx, sp
	mov [bx], ax
	call get_cursor
	mov ax, [bp-2]
	mov bl, 80
	div bl
	mov ah, 0
	mov bx, 80
	mul bx
	add ax, 80
	mov bx, sp
	mov [bx], ax
	call set_cursor
	add sp, 4
	pop bp
	ret

; csr_wrap()
csr_wrap:
	push bp
	mov bp, sp
	sub sp, 4
	lea ax, [bp-2]
	mov bx, sp
	mov [bx], ax
	call get_cursor
	mov ax, [bp-2]
	add ax, 80
	mov bx, sp
	mov [bx], ax
	call set_cursor
	add sp, 4
	pop bp
	ret

;csr_return()
csr_return:
	push bp
	mov bp, sp
	sub sp, 4
	lea ax, [bp-2]
	mov bx, sp
	mov [bx], ax
	call get_cursor
	mov ax, [bp-2]
	mov bl, 80
	div bl
	mov ah, 0
	mov bx, 80
	mul bx
	mov bx, sp
	mov [bx], ax
	call set_cursor
	add sp, 4
	pop bp
	ret

;csr_bksp()
csr_bksp:
	push bp
	mov bp, sp
	sub sp, 4
	lea ax, [bp-2]
	mov bx, sp
	mov [bx], ax
	call get_cursor
	mov ax, [bp-2]
	mov bx, 0
	cmp ax, bx
	jz L1
	sub ax, 1
L1:
	mov bx, sp
	mov [bx], ax
	call set_cursor
	add sp, 4
	pop bp
	ret

; putstra(uint_16* str_attr)
putstra:
	push bp
	mov bp, sp
	sub sp, 4
.L1:
	mov bx, [bp+4]
	mov ax, [bx]
	mov bl, 0
	cmp al, bl
	jz .end
	mov bl, 0x0a
	cmp al, bl
	jnz .L2
	call nxtl
.L2:
	mov bx, sp
	mov [bx], ax
	call putchara
	mov ax, [bp+4]
	add ax, 2
	mov [bp+4], ax
	jmp .L1
.end:
	add sp, 4
	pop bp
	ret

; putstr(uint_16* str)
putstr:
	push bp
	mov bp, sp
	sub sp, 4
.L1:
	mov bx, [bp+4]
	mov al, [bx]
	mov bl, 0
	cmp al, bl
	jz .end
	mov bl, 0x0a
	cmp al, bl
	jnz .L2
	call nxtl
	mov ax, [bp+4]
	add ax, 1
	mov [bp+4], ax
	jmp .L1
.L2:
	mov si, sp
	mov byte [si+1], 0
	mov byte [si], al
	call putchar
	mov ax, [bp+4]
	add ax, 1
	mov [bp+4], ax
	jmp .L1
.end:
	add sp, 4
	pop bp
	ret

; putfhex8(val)
putfhex8:
	push bp
	mov bp, sp
	sub sp, 2

	mov ax, [bp+4]
	mov bx, 0x00f0
	and ax, bx
	shr ax, 4
	mov bx, str_hex
	add bx, ax
	xor ax, ax
	mov al, [bx]
	mov bx, sp
	mov [bx], ax
	call putchar
	mov ax, [bp+4]
	mov bx, 0x000f
	and ax, bx
	mov bx, str_hex
	add bx, ax
	xor ax, ax
	mov al, [bx]
	mov bx, sp
	mov [bx], ax
	call putchar
	add sp, 2
	pop bp
	ret

; putfhex16(val)
putfhex16:
	push bp
	mov bp, sp
	sub sp, 2
	
	mov ax, [bp+4]
	shr ax, 8
	mov bx, sp
	mov [bx], ax
	call putfhex8
	mov ax, [bp+4]
	mov bx, sp
	mov [bx], ax
	call putfhex8

	add sp, 2
	pop bp
	ret

; disk_read(16bit count, 16bit addr, 16bit seg, 32bit blocklba)
disk_read:
	push bp
	mov bp, sp
	push ds
	sub sp, 8
	
	mov dx, 0x3f6
	mov al, 0
	out dx, al
	mov dx, 0x1f0
	mov ax, 0
	out dx, al
	
	mov dx, 0x3f7
	mov al, 0x
	out dx, al
	mov dx, 0x1f1
	mov al, 0
	out dx, al
	mov dx, 0x1f7
	mov al, 0xe7
	out dx, al
	mov dx, 0x1f6
	mov al, 0xe0
	out dx, al

	mov ax, [bp+12]
	mov dx, 0x1f2
	out dx, al
	mov ax, [bp+4]
	mov dx, 0x1f3
	out dx, al
	shr ax, 8
	mov dx, 0x1f4
	out dx, al
	mov ax, [bp+6]
	mov dx, 0x1f5
	out dx, al
	shr ax, 8
	and al, 0x0f
	or al, 0xe0
	mov dx, 0x1f6
	out dx, al
	mov dx, 0x1f7
	mov al, 0x20
	out dx, al
	jmp .not_ready
.not_ready:
	mov dx, 0x1f7
	mov al, 0xec
	out dx, al
	nop
	nop
	nop
	mov dx, 0x1f7
	in al, dx
	and al, 0x88
	cmp al, 0x08
	jnz .not_ready
.suc:
	mov cx, [bp+10]
	shl cx, 8
	mov bx, [bp+8]
	mov ds, bx
	mov bx, [bp+10]
.cont_read:
	mov dx, 0x1f0
	in ax, dx
	mov [ds:bx], ax
	add bx, 2
	loop .cont_read
	add sp, 8
	pop ds
	pop bp
	ret

global_init:
	mov ax, 0xb800
	mov es, ax

	mov cx, 2000
	mov si, 0
	loop1:
	mov word [es:si], 0x0700
	add si, 2
	loop loop1

;	push bp
;	mov bp, sp

	sub sp, 16

	mov ax, 0
	mov bx, sp
	mov [bx], ax
	call set_cursor

	mov bx, sp
	mov ax, 0x010a
	mov [bx], ax
	add bx, 2
	mov ax, 0x0000
	mov [bx], ax
	add bx, 2
	mov ax, 0x0000
	mov [bx], ax
	add bx, 2
	mov ax, 0x0500
	mov [bx], ax
	add bx, 2
	mov ax, 0x0001
	mov [bx], ax
	call disk_read

	mov bx, 0x6fe
	mov al, [bx]
	mov byte [es:0], al

	mov ax, [_magic]
	jmp $

_magic: dw 0x6666
str_hex: db "0123456789ABCDEF"
str1: dw 0x0741, 0x0742, 0x0743, 0x0744, 0x0724
str2: db "LWOS", 0xa, "FINEST$"
