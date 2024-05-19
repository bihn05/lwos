org 0x7c00
jmp init

db "LWSOFT24"			;OEM ID
dw 0x0200				;bytes per sector
dd 0x00000800			;number of sectors
db 0x3f					;media
dd 0x00000080			;sectors of FAT
dd 0x00000080			;sectors of FDT
dw 0x0001				;sectors per cluster
db "FILESYSTEMTEST00"	;volume label
dd 0xffffffff			;volume serial number
db "LWFAT32 "			;file system name

init:
	mov ax, 0
	mov ss, ax
	mov ds, ax
	mov sp, 0x7c00

	call read_disk

success:
	mov si, boot_success
	call print

	jmp 0x500

read_disk:
	mov dx, 0x1f2
	mov al, 1
	out dx, al

	mov dx, 0x1f3
	mov al, 0x0a
	out dx, al

	mov dx, 0x1f4
	mov al, 0x01
	out dx, al

	mov dx, 0x1f5
	mov al, 0x00
	out dx, al

	mov dx, 0x1f6
	mov al, 0xe0
	out dx, al

	mov dx, 0x1f7
	mov al, 0x20
	out dx, al

.not_ready:
	nop
	nop
	nop
	in al, dx
	and al, 0x88
	cmp al, 0x08
	jnz .not_ready

	mov ax, 0x100
	mov cx, ax
	mov dx, 0x1f0
	mov bx, 0x500
.go_on_read:
	in ax, dx
	mov [ds:bx], ax
	add bx, 2
	loop .go_on_read
	ret


error:
	mov si, boot_failed
	call print
	.halt:
	hlt
	jmp .halt
	
print:
	mov al, [si]
	add si, 1
	cmp al, 0
	je .rtn
	mov ah, 0x0e
	mov bx, 0x0f
	int 0x10
	jmp print
	.rtn:
	ret
	
boot_success:
	db "Loading", 0
boot_failed:
	db "Bad media", 0
error_count:
	dw 0

	times 510-($-$$) db 0
	dw 0xaa55
