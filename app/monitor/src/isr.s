[bits 32]
section .text
global isr_table
global dbg_enter
global cur_ctx
global in_debuggee
extern dbg_panic

%macro ISR_ERR 1
isr%1:
    push %1
    jmp isr_common
%endmacro

%macro ISR_NOERR 1
isr%1:
    push 0
    push %1
    jmp isr_common
%endmacro

%assign v 0
%rep 48
	%if v == 8 || v == 17 || v == 21 || (v >= 10 && v <= 14)
		ISR_ERR v
	%else
		ISR_NOERR v
	%endif
	%assign v v+1
%endrep

isr_common:
    pushad
    push ds
    push es
    push fs
    push gs
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    ;  +0h: gs
    ;  +4h: fs
    ;  +8h: es
    ;  +ch: ds
    ; +10h: edi
    ; +14h: esi
    ; +18h: ebp
    ; +1ch: esp(org)
    ; +20h: ebx
    ; +24h: edx
    ; +28h: ecx
    ; +2ch: eax
    ; +30h: vector
    ; +34h: errcode
    ; +38h: eip
    ; +3ch: cs
    ; +40h: eflags
    mov ebx, [cur_ctx]

    mov eax, [esp + 0x2c]
    mov [ebx + 0x0], eax
    mov eax, [esp + 0x28]
    mov [ebx + 0x4], eax
    mov eax, [esp + 0x24]
    mov [ebx + 0x8], eax
    mov eax, [esp + 0x20]
    mov [ebx + 0xc], eax
    mov eax, [esp + 0x18]
    mov [ebx + 0x14], eax
    mov eax, [esp + 0x14]
    mov [ebx + 0x18], eax
    mov eax, [esp + 0x10]
    mov [ebx + 0x1c], eax

    mov eax, [esp + 0x30]
    mov [ebx + 0x40], eax
    mov eax, [esp + 0x34]
    mov [ebx + 0x44], eax
    mov eax, [esp + 0x38]
    mov [ebx + 0x20], eax
    mov eax, [esp + 0x3c]
    mov [ebx + 0x28], eax
    mov eax, [esp + 0x40]
    mov [ebx + 0x24], eax

    lea eax, [esp + 0x44]
    mov [ebx + 0x10], eax
    mov eax, ss
    mov [ebx + 0x3c], eax
    mov eax, [esp + 0xc]
    mov [ebx + 0x2c], eax
    mov eax, [esp + 0x8]
    mov [ebx + 0x30], eax
    mov eax, [esp + 0x4]
    mov [ebx + 0x34], eax
    mov eax, [esp + 0x0]
    mov [ebx + 0x38], eax

    cmp dword [in_debuggee], 0
    je .in_monitor

    mov dword [in_debuggee], 0
    mov esp, [mon_esp]
    popad
    ret
.in_monitor:
    push ebx
    call dbg_panic
.hang:
    cli
    hlt
    jmp .hang

; void dbg_enter(struct ctx *c)
dbg_enter:
    pushad
    mov [mon_esp], esp
    mov ebx, [esp+0x24]
    mov [cur_ctx], ebx
    mov dword [in_debuggee], 1

    mov eax, [ebx+0x10]
    sub eax, 12
    mov edx, [ebx+0x24]
    mov [eax+0x8], edx
    mov edx, [ebx+0x28]
    mov [eax+0x4], edx
    mov edx, [ebx+0x20]
    mov [eax], edx

    mov ecx, [ebx+0x2c]
    mov ds, cx
    mov ecx, [ebx+0x30]
    mov es, cx
    mov ecx, [ebx+0x34]
    mov fs, cx
    mov ecx, [ebx+0x38]
    mov gs, cx

    mov ecx, [ebx+0x4]
    mov edx, [ebx+0x8]
    mov esi, [ebx+0x18]
    mov edi, [ebx+0x1c]
    mov ebp, [ebx+0x14]
    mov esp, eax
    mov eax, [ebx+0]
    mov ebx, [ebx+0xc]
    iret

section .data
align 4
in_debuggee: dd 0
mon_esp: dd 0
cur_ctx: dd panic_ctx

isr_table:
%assign v 0
%rep 48
    dd isr %+ v
    %assign v v+1
%endrep

section .bss
align 4
panic_ctx: resb 72