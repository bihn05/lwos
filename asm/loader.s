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

; --- 新增：64位 GDT ---
gdt64_start:
	dq 0x0000000000000000 ; Null 描述符
	dq 0x0020980000000000 ; 64位代码段 (L=1, D/B=0)
	dq 0x0000920000000000 ; 64位数据段
gdt64_end:

gdt64_descriptor:
	dw gdt64_end - gdt64_start - 1
	dd gdt64_start
; ---------------------

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
.halt:
	hlt
	jmp .halt

str_3 db 'Memory detection error', 0
str_4 db 'Memory OK, Booting...', 0

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
	mov si, str_4
	call print

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
	mov esp, 0x10000
	mov ebp, esp

	; 屏蔽 8259A PIC 中断 (为后续 64 位环境做准备)
	mov al, 0xff
	out 0x21, al
	out 0xa1, al
	
	; 1. 在 32位平坦模式下 (未开启分页) 读取内核到 0x10000
	push dword 0x10000
	push dword 0
	push dword 0x10
	push dword 200
	push dword 8728
	call read_scts
	add esp, 0x14

	call cls

	push dword str
	call putstr
	add esp, 0x4

	; 2. 准备 64位 4级页表 (PML4 -> PDPT -> PD，使用 2MB 巨页恒等映射 0~2MB)
	; 清理 0x100000 ~ 0x102FFF (12KB) 作为页表空间
	mov edi, 0x100000
	mov ecx, 3072        ; 3072 个 dword = 12KB
	xor eax, eax
	rep stosd

	; 设置页表条目
	; mov dword [0x100000], 0x101003 ; PML4[0] 指向 PDPT (Present + R/W)
	; mov dword [0x101000], 0x102003 ; PDPT[0] 指向 PD   (Present + R/W)
	; mov dword [0x102000], 0x000083 ; PD[0] 映射 0x0 开始的 2MB 巨页 (Present + R/W + Huge)

; 设置 PML4 和 PDPT 的第一项
	mov dword [0x100000], 0x101003 ; PML4[0] 指向 PDPT (Present + R/W)
	mov dword [0x101000], 0x102003 ; PDPT[0] 指向 PD   (Present + R/W)

	; 循环填充 PD 的 512 个条目，映射前 1GB 物理内存
	mov edi, 0x102000    ; PD 基址
	mov eax, 0x00000083  ; 初始值：物理地址 0, Present, R/W, Huge Page (2MB)
	mov ecx, 512         ; 循环 512 次
.map_1gb:
	mov [edi], eax       ; 写入低 32 位 (地址 + 属性)
	mov dword [edi+4], 0 ; 写入高 32 位 (设为 0)
	add eax, 0x200000    ; 每次增加 2MB (物理基址递增)
	add edi, 8           ; 每次向后移动 8 个字节 (64位页表条目占 8 字节)
	loop .map_1gb

	; 3. 开启 PAE (Physical Address Extension)
	mov eax, cr4
	or eax, 1 << 5
	mov cr4, eax

	; 4. 加载 PML4 基址到 CR3
	mov eax, 0x100000
	mov cr3, eax

	; 5. 开启 EFER 寄存器中的 LME (Long Mode Enable) 标志
	mov ecx, 0xC0000080
	rdmsr
	or eax, 1 << 8
	wrmsr

	; 6. 开启分页 (这一步真正激活了长模式兼容环境)
	mov eax, cr0
	or eax, 1 << 31
	mov cr0, eax

	; 7. 加载 64位 GDT
	lgdt [gdt64_descriptor]

	; 8. 远跳转，彻底进入 64 位长模式
	jmp 0x08:long_mode_start

[bits 64]
long_mode_start:
	; 在 64 位下初始化数据段
	mov ax, 0x10 
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

    mov rsp, 0x90000
    mov rbp, rsp

	; 华丽的纵身一跃，跳入 64位 内核
	mov rax, 0x10000
	jmp rax

; ==========================================
; 以下为原有的 32 位子程序 (保持完全不变)
; ==========================================
[bits 32]
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
	push str_err
	call putstr
	add esp, 0x4
	jmp $
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
	mov [cursor_x], al
	mov [cursor_y], al
	xor eax, eax
	mov esi, 0xb8000
	mov ecx, 2000
.loop1:
	mov word [esi], ax
	add esi, 2
	loop .loop1
	add esp, 0x10
	pop ebp
	ret

screen_roll:
	push ebp
	mov ebp, esp
	sub esp, 0x10
	mov ecx, 1920
.loop1:
	mov esi, 0xb80a0
	mov edi, 0xb8000
	mov edx, [esi]
	mov [edi], edx
	inc esi
	inc edi
	loop .loop1
	add esp, 0x10
	pop ebp
	ret

putchar:
	push ebp
	mov ebp, esp
	sub esp, 0x10
	xor eax, eax
	mov al, [esp+0x18]
	mov bl, 0x8
	cmp al, bl
	jne .next
	mov al, [cursor_x]
	cmp al, 0
	jne .next1
	jmp .done
.next1:
	dec byte [cursor_x]
	jmp .done
.next:
	mov al, [esp+0x18]
	mov bl, 0xd
	cmp al, bl
	jne .next2
	inc byte [cursor_y]
	mov al, [cursor_y]
	mov bl, 25
	call screen_roll
	mov bl, 24
	jmp .done
.next2:
	mov al, [esp+0x18]
	mov bl, 0xa
	cmp al, bl
	jne .next3
	mov byte [cursor_x], 0
	jmp .done
.next3:
	mov al, [cursor_x]
	mov bl, 79
	cmp al, bl
	jne .next4
	mov byte [cursor_x], 0
	inc byte [cursor_y]
	mov al, [cursor_y]
	mov bl, 25
	cmp al, bl
	jne .next4
	call screen_roll
	mov bl, 24
.next4:
	mov al, [cursor_y]
	mov bl, 80
	mul bl
	mov bx, [cursor_x]
	add ax, bx
	shl ax, 1
	mov [esp], eax
	mov esi, 0xb8000
	xor eax, eax
	mov eax, [esp]
	add esi, eax
	mov dl, [esp+0x18]
	mov ax, [char_attr]
	or dx, ax
	mov [esi], dx
	inc byte [cursor_x]
.done:
	add esp, 0x10
	xor eax, eax
	pop ebp
	ret

putstr:
	push ebp
	mov ebp, esp
	sub esp, 0x10
	mov esi, [esp+0x18]
.loop:
	mov al, [esi]
	mov bl, 0
	cmp al, bl
	je .done
	xor eax, eax
	mov al, [esi]
	mov [esp], esi
	push eax
	call putchar
	add esp, 0x4
	mov esi, [esp]
	inc esi
	jmp .loop
.done:
	xor eax, eax
	add esp, 0x10
	pop ebp
	ret

cursor_x: db 0
cursor_y: db 0
char_attr: dw 0x0F00

str: db "lwloader working. . .", 0
str_err: db "##BAD MEDIA", 0