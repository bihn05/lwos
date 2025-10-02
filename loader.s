[org 0x900]
jmp init

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

mem_err: 
	mov si, str_3
	call print
	jmp $

str_3 db 'Memory detection error', 0

init:
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

	cli
	
	in al, 0x92
	or al, 0x02
	out 0x92, al

	lgdt [gdt_descriptor]

	mov eax, cr0
	or eax, 1
	mov cr0, eax

	jmp dword 0x08:pmode_start
[bits 32]
pmode_start:
	mov eax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	mov esp, 0x90000
	mov ebp, esp
	
setup_page:
	xor eax, eax
	mov ecx, 1024
	mov edi, 0x100000
.L1:
	mov dword [edi], eax
	add edi, 4
	loop .L1
;	jmp $

	mov eax, 0x00100007
	mov dword [0x00100ffc], eax
	add eax, 0x1000
	mov dword [0x00100000], eax
	add eax, 0x1000
	mov dword [0x00100004], eax

	mov ecx, 0x800
	mov esi, 0
	mov edx, 0x00000007
	mov ebx, 0x00101000
.create_pte:
	mov dword [ebx+esi*4], edx
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

	push dword 0x200000
	push dword 0
	push dword 0x10
	push dword 80
	push dword 261
	call read_scts
	add esp, 0x14
	xchg bx, bx

	call cls

	jmp $

wait_drive_ready:
	push ebp
	mov ebp, esp
	sub esp, 0x10
	mov dword [esp], 100000
	mov ecx, [esp]
.loop1:
	mov dx, 0x1f7
	in al, dx
	and al, 0x80
	cmp al, 0x80
	jne .ready
	loop .loop1
	mov eax, 0xffffffff
	jmp .done
.ready:
	mov eax, 0
.done:
	add esp, 0x10
	pop ebp
	ret

wait_data_ready:
	push ebp
	mov ebp, esp
	sub esp, 0x10
	mov dword [esp], 100000
	mov ecx, [esp]
.loop1:
	mov dx, 0x1f7
	in al, dx
	mov [esp+4], al
	and al, 0x01
	cmp al, 0x01
	jne .next
	mov eax, 0xffffffff
	jmp .done
	mov al, [esp+4]
	and al, 0x08
	cmp al, 0x08
	jne .next
	mov eax, 0
	jmp .done
.next:
	loop .loop1
	mov eax, 0xffffffff
.done:
	add esp, 0x10
	pop ebp
	ret

read_sct:
	push ebp
	mov ebp, esp
	sub esp, 0x10
	mov dword [esp], 1
	mov eax, [esp+0x18]
	mov dword ebx, 0xffffff
	cmp eax, ebx
	jc .next1
	mov eax, 0xffffffff
	jmp .done
.next1:
	call wait_drive_ready
	cmp eax, 0
	je .next2
	mov eax, 0xfffffffe
	jmp .done
.next2:
	mov byte [esp+0x4], 0xe0
	mov al, [esp+0x20]
	and al, 0x01
	mov cl, 4
	shl al, cl
	or [esp+0x4], al
	mov al, [esp+0x1b]
	and al, 0x0f
	or [esp+0x4], al
	mov al, [esp+0x4]
	mov dx, 0x1f6
	out dx, al
	mov al, [esp]
	mov dx, 0x1f2
	out dx, al
	mov al, [esp+0x18]
	mov dx, 0x1f3
	out dx, al
	mov al, [esp+0x19]
	mov dx, 0x1f4
	out dx, al
	mov al, [esp+0x1a]
	mov dx, 0x1f5
	out dx, al
	mov al, 0x20
	mov dx, 0x1f7
	out dx, al
	call wait_data_ready
	cmp eax, 0
	jne .next3
	mov eax, 0xfffffffd
	jmp .done
.next3:
	mov dx, 0x1f0
	mov edi, [esp+0x24]
	mov ecx, 256
	cld
	rep insw
	mov eax, 0
.done:
	add esp, 0x10
	pop ebp
	ret

read_scts:
	push ebp
	mov ebp, esp
	sub esp, 0x10
	mov dword [esp], 0
	mov eax, [esp+0x18]
	mov [esp+0x4], eax
	mov eax, [esp+0x28]
	mov [esp+0x8], eax
.loop1:
	sub esp, 0x10
	mov eax, [esp+0x14]
	mov [esp], eax
	mov eax, [esp+0x30]
	mov [esp+0x4], eax
	mov eax, [esp+0x34]
	mov [esp+0x8], eax
	mov eax, [esp+0x18]
	mov [esp+0xc], eax
	xor eax, eax
	call read_sct
	add esp, 0x10
	xor ebx, ebx
	cmp eax, ebx
	jne .err
	inc dword [esp]
	mov eax, [esp]
	mov ebx, [esp+0x1c]
	cmp eax, ebx
	je .ok
	inc dword [esp+0x4]
	add dword [esp+0x8], 0x200
	jmp .loop1
.err:
	mov eax, [esp+0xc]
	jmp .done
.ok:
	mov eax, 0
	jmp .done
.done:
	add esp, 0x10
	pop ebp
	ret

memcpy:
	push ebp
	mov ebp, esp
	sub esp, 0x10
	mov ecx, [esp+0x20]
	mov esi, [esp+0x1c]
	mov edi, [esp+0x18]
.loop:
	mov dl, [esi]
	mov [edi], dl
	inc esi
	inc edi
	loop .loop
	add esp, 0x10
	mov eax, 0
	pop ebp
	ret
	
cls:
	push ebp
	mov ebp, esp
	sub esp, 0x10
	xor eax, eax
	mov [cursor_x], eax
	mov [cursor_y], eax
	mov esi, 0xb8000
	mov ecx, 2000
.loop1:
	mov word [esi], ax
	add esi, 2
	loop .loop1
	add esp, 0x10
	pop ebp
	ret

putchar:
	push ebp
	mov ebp, esp
	sub esp, 0x10
	mov al, [cursor_y]
	mov bl, 80
	mul bl
	mov bx, [cursor_x]
	add ax, bx
	shl ax, 2
	mov [esp], ax
	mov esi, 0xb8000
	xor eax, eax
	mov eax, [esp]
	add esi, eax
	mov dx, [esp+0x18]
	mov [esi], dx
	mov dx, [char_attr]
	inc esi
	mov [esi], dx
	add esp, 0x10
	xor eax, eax
	pop ebp
	ret

cursor_x: db 0
cursor_y: db 0
char_attr: db 0x07