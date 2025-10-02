
[bits 32]
section .text

extern _interrupt_handler
extern _irq_handler

%macro ISR_NOERRCODE 1
global _isr%1
_isr%1:
	cli
	push dword 0
	push dword %1
	jmp isr_common
%endmacro

%macro ISR_ERRCODE 1
global _isr%1
_isr%1:
	cli
	push dword %1
	jmp isr_common
%endmacro

%macro IRQ 2
global _irq%1
_irq%1:
	cli
	push dword 0
	push dword %2
	jmp irq_common
%endmacro

; 无错误码的中断 (0-31)
ISR_NOERRCODE 0   ; 除零错误
ISR_NOERRCODE 1   ; 调试异常
ISR_NOERRCODE 2   ; 不可屏蔽中断
ISR_NOERRCODE 3   ; 断点异常
ISR_NOERRCODE 4   ; 溢出异常
ISR_NOERRCODE 5   ; 越界异常
ISR_NOERRCODE 6   ; 无效操作码
ISR_NOERRCODE 7   ; 设备不可用
ISR_ERRCODE    8   ; 双重错误（有错误码）
ISR_NOERRCODE 9   ; 协处理器段溢出
ISR_ERRCODE    10  ; 无效TSS（有错误码）
ISR_ERRCODE    11  ; 段不存在（有错误码）
ISR_ERRCODE    12  ; 栈段错误（有错误码）
ISR_ERRCODE    13  ; 一般保护错误（有错误码）
ISR_ERRCODE    14  ; 页错误（有错误码）
ISR_NOERRCODE 15  ; 保留
ISR_NOERRCODE 16  ; 浮点错误
ISR_NOERRCODE 17  ; 对齐检查
ISR_NOERRCODE 18  ; 机器检查
ISR_NOERRCODE 19  ; SIMD浮点异常

; IRQ 中断 (32-47)
IRQ 0, 32   ; 定时器中断
IRQ 1, 33   ; 键盘中断
IRQ 2, 34   ; 级联中断
IRQ 3, 35   ; COM2
IRQ 4, 36   ; COM1
IRQ 5, 37   ; LPT2
IRQ 6, 38   ; 软盘
IRQ 7, 39   ; LPT1
IRQ 8, 40   ; CMOS实时钟
IRQ 9, 41   ; 自由中断
IRQ 10, 42  ; 自由中断
IRQ 11, 43  ; 自由中断
IRQ 12, 44  ; PS2鼠标
IRQ 13, 45  ; 协处理器
IRQ 14, 46  ; 主硬盘
IRQ 15, 47  ; 从硬盘

isr_common:
	pushad

	push ds
	push es
	push fs
	push gs

	mov eax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	
;	jmp $

	push esp
	call _interrupt_handler
	
	add esp, 4

	pop gs
	pop fs
	pop es
	pop ds

	popad

	add esp, 8

	sti
	iret

irq_common:
	pushad
	push ds
	push es
	push fs
	push gs

	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	push esp
	call _irq_handler
	add esp, 4

	pop gs
	pop fs
	pop es
	pop ds
	popad

	add esp, 4

	sti
	iret
