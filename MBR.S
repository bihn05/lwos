[org 0x7c00]
jmp init

db 0
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
	mov es, ax
	mov sp, 0x7c00
	mov bp, sp

	mov ah, 6
	mov al, 0
	mov ch, 0
	mov cl, 0
	mov dh, 24
	mov dl, 79
	mov bh, 0x07
	int 0x10

	mov ah, 2
	mov dx, 0
	mov bh, 0
	int 0x10

	mov si, str1
	call print
	
	mov ah, 0
	mov dl, 0x80
	int 0x13
	jc disk_err

	mov ah, 0x02
	mov al, 0x10 ;sctmnt
	mov ch, 0x00 ;cylinder
	mov cl, 0x0f ;sector
	mov dh, 0x04 ;head
	mov dl, 0x80 ;device
	mov bx, 0x700 ;[es:bx]
	int 0x13
	jc disk_err

	jmp 0x700

	jmp $

disk_err:
	mov si, str2
	call print
	hlt

print:
.L1:
    mov al, [si]
	inc si
    cmp al, 0 
    je .done  
    mov ah, 0x0e
    int 0x10
    jmp .L1
.done:
    ret

str1:
db "MBR IS WORKING WELL", 0xd, 0xa, 0
str2:
db "DISK ERROR OCURRED", 0xd, 0xa, 0

	times 510-($-$$) db 0
	dw 0xaa55
