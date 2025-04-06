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
	mov sp, 0x7c00
	mov bp, sp

	mov si, str1
	call print
	
	mov ah, 0
	mov dl, 0x80
	int 0x13
	jc disk_err

	mov ax, 0x7000
	mov es, ax
	mov ah, 0x02
	mov al, 0x60 ;sctmnt
	mov ch, 0x00 ;cylinder
	mov cl, 0x0a ;sector
	mov dh, 0x04 ;head
	mov dl, 0x80 ;device
	mov bx, 0x00 ;[es:bx]
	int 0x13
	jc disk_err

detect_mem:
	xor ebx, ebx
	mov es, bx
	mov edi, 0x7e10 ; es:di struct buffer ptr
	mov edx, 0x534d4150

.next:
	mov eax, 0xe820
	mov ecx, 24
	int 0x15
	jc mem_err
	add di, cx
	inc word [0x7e00]
	cmp ebx, 0
	jnz .next

	call cnt
	
disk_err:
	mov si, str2
	call print
	hlt
mem_err:
	mov si, str3
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

cnt:
;	mov byte [es:0], 0xeb
;	mov byte [es:1], 0xfe
	cli
	mov al, 0xdd
	out 0x64, al
	pop ax

	lgdt [gdt_descriptor]

	mov eax, cr0
	or eax, 0x1
	mov cr0, eax

	jmp dword 0x08:pmode_start
[bits 32]
pmode_start:
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov ss, ax
	mov gs, ax

setup_page:
	xor eax, eax
	mov ecx, 1024
	mov edi, 0x00100000
.L1:
	mov dword [edi], eax
	add edi, 4
	loop .L1

	mov eax, 0x00100007
	mov dword [0x100ffc], eax
	add eax, 0x1000
	mov dword [0x100000], eax
	add eax, 0x1000
	mov dword [0x100004], eax

	mov ecx, 0x800
	mov esi, 0
	mov edx, 0x00000007
	mov ebx, 0x101000
.create_pte:
	mov [ebx+esi*4], edx
	add edx, 0x1000
	inc esi
	loop .create_pte

	mov eax, 0x100000
	mov cr3, eax ; page pos
	mov eax, cr0
	or eax, 0x80000000
	mov cr0, eax ; setup page
	mov eax, cr3
	mov cr3, eax

	lgdt [gdt_descriptor]

	xchg bx, bx

	jmp 0x70000 ; to

gdt_start:
	;nul
	dd 0x00000000
	dd 0x00000000

	; code segment
	dd 0x0000ffff
	dd 0x00cf9a00

	; data segment
	dd 0x0000ffff
	dd 0x00cf9200
gdt_end:

gdt_descriptor:
	dw gdt_end - gdt_start - 1
	dd gdt_start

str1:
db "MBR IS WORKING WELL", 0xd, 0xa, 0
str2:
db "MEDIA ERR", 0xd, 0xa, 0
str3:
db "MEM ERR", 0xd, 0xa, 0

	times 510-($-$$) db 0
	dw 0xaa55
