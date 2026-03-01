[org 0x7c00]
jmp init

db 0
fs_name: db "LWFAT32 "
db 0,0,0,0,0
partition_offset: dq 0
partition_length: dq 251658240
fat_offset: dd 8
fat_length: dd 8192
cluster_offset: dd 8200
cluster_count: dd 512
root_cluster: dd 2
volume_serial_number: dd 0x20050605
fs_version: dw 0x1000
sector_shift: db 9
clustor_shift: db 3
volume_flags: db 0
drive_select: db 0x80
used_in_percent: db 0
db 0

init:
	mov ax, 0
	mov ss, ax
	mov ds, ax
	mov sp, 0x7c00
	mov bp, sp

	mov si, str_1
	call print

	mov ah, 0
	mov dl, 0x80
	int 0x13
	jc disk_err

	mov ax, 0x0000
	mov es, ax
	mov ah, 0x02
	mov al, 0x08 ; 8*512=4KB
	mov ch, 0x00 ; ����
	mov cl, 0x02 ; ����
	mov dh, 0x00 ; ��ͷ
	mov dl, 0x80 ; �豸
	mov bx, 0x0900 ; [es:bx]
	int 0x13
	jc disk_err

	jmp 0x900

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

disk_err:
	mov si, str_2
	call print
	hlt

str_1: db 'LWOS MBR OK ', 0
str_2: db 'MEDIA ERR', 0

times 510-($-$$) db 0
dw 0xAA55