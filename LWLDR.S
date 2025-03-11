section loader vstart=0x700

jmp global_init

;idt_start:
;	times 256 dd 0, 0
;idt_end:

;idt_descriptor:
;	dw idt_end - idt_start - 1
;	dd idt_start

gdt_start:
	; nul
	dd 0x00000000
	dd 0x00000000

	; code segment
	dd 0x0000ffff
	dd 0x00cf9a00

	; data segment
	dd 0x0000ffff
	dd 0x00cf9200

	; video segment
	dd 0x80000007
	dd 0x00c0920b

	; stack segment
	dd 0x0000ffff
	dd 0x00cf9200
gdt_end:

gdt_descriptor:
	dw gdt_end - gdt_start - 1
	dd gdt_start

global_init:

	cli

	push ax
	mov al, 0xdd
	out 0x64, al ; a20
	pop ax

	lgdt [gdt_descriptor]

	mov eax, cr0
	or eax, 0x1
	mov cr0, eax

	jmp dword 0x08:protected_mode_start

[bits 32]
protected_mode_start:
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov ax, 0x18
	mov gs, ax
	mov ax, 0x20
	mov ss, ax

	sgdt [gdt_descriptor]

	mov dx, 0x1f3
	mov al, 0x0e
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

	mov dx, 0x1f2
	mov al, 0x40 ; how many sct
	out dx, al

	mov dx, 0x1f7
	mov al, 0x20
	out dx, al

.wait_ready:
	in al, dx
	test al, 0x80
	jnz .wait_ready

	mov ecx, 0x2000
	mov edi, 0x00070000
	mov dx, 0x1f0

.read_loop:
	in ax, dx
	mov [edi], ax
	add edi, 2
	loop .read_loop

setup_page:
	xor eax, eax
	mov ecx, 1024
	mov edi, 0x00100000
.L1:
	mov dword [edi], eax
	add edi, 4
	loop .L1


	mov eax, 0x00101007
	mov dword [0x100000], eax

	mov eax, 0x00102007
	mov dword [0x100004], eax

;	mov dword [0x100c00], eax
	mov eax, 0x00100007
	mov dword [0x100ffc], eax

	mov ecx, 0x800
	mov esi, 0
	mov edx, 0x00000007
	mov ebx, 0x101000
.create_pte_1:
	mov [ebx+esi*4], edx
	add edx, 0x1000 ; 4KB * 0x800 = 8MB
	inc esi
	loop .create_pte_1

	mov eax, 0x100000 ; pd pos
        mov cr3, eax

        mov eax, cr0
        or eax, 0x80000000
        mov cr0, eax

	xchg bx, bx

	mov dword [0x710 + 0x8], 0x0000ffff
	mov dword [0x710 + 0xc], 0x00cf9a00
        mov dword [0x710 + 0x10], 0x0000ffff
        mov dword [0x710 + 0x14], 0x00cf9200
        mov dword [0x710 + 0x18], 0x80000007
        mov dword [0x710 + 0x1c], 0x00c0920b
        mov dword [0x710 + 0x20], 0x0000ffff
        mov dword [0x710 + 0x24], 0x00cf9200

	mov word [0x700], 0x23
	mov dword [0x702], 0x710

	lgdt [0x700]

;	or dword [gdt_start + 0x18 + 4], 0xc0000000
;	add dword [gdt_descriptor + 2], 0xc0000000
;	add esp, 0xc0000000

;	lgdt [gdt_descriptor]

	jmp 0x0008:0x00070000
