[bits 32]

extern _handler_table

%macro INTERRUPT_HANDLER 2
interrupt_handle_%1:
%ifn %2
	push 0xe1eccafe
%endif
	push %1
	jmp interrupt_entry
%endmacro

interrupt_entry:
	push ds
	push es
	push fs
	push gs

	mov eax, [esp+12*4]
	push eax
	call [_handler_table+eax*4]

global _interrupt_exit
_interrupt_exit:
	add esp, 4
	call task_signal
	popa
	pop gs
	pop fs
	pop es
	pop ds

	add esp, 8
	iret

INTERRUPT_HANDLER 0x00, 0; divide by zero
INTERRUPT_HANDLER 0x01, 0; debug
INTERRUPT_HANDLER 0x02, 0; non maskable interrupt
INTERRUPT_HANDLER 0x03, 0; breakpoint

INTERRUPT_HANDLER 0x04, 0; overflow
INTERRUPT_HANDLER 0x05, 0; bound range exceeded
INTERRUPT_HANDLER 0x06, 0; invalid opcode
INTERRUPT_HANDLER 0x07, 0; device not avilable

INTERRUPT_HANDLER 0x08, 1; double fault
INTERRUPT_HANDLER 0x09, 0; coprocessor segment overrun
INTERRUPT_HANDLER 0x0a, 1; invalid TSS
INTERRUPT_HANDLER 0x0b, 1; segment not present

INTERRUPT_HANDLER 0x0c, 1; stack segment fault
INTERRUPT_HANDLER 0x0d, 1; general protection fault
INTERRUPT_HANDLER 0x0e, 1; page fault
INTERRUPT_HANDLER 0x0f, 0; reserved

INTERRUPT_HANDLER 0x10, 0; x87 floating point exception
INTERRUPT_HANDLER 0x11, 1; alignment check
INTERRUPT_HANDLER 0x12, 0; machine check
INTERRUPT_HANDLER 0x13, 0; SIMD Floating - Point Exception

INTERRUPT_HANDLER 0x14, 0; Virtualization Exception
INTERRUPT_HANDLER 0x15, 1; Control Protection Exception
INTERRUPT_HANDLER 0x16, 0; reserved
INTERRUPT_HANDLER 0x17, 0; reserved

INTERRUPT_HANDLER 0x18, 0; reserved
INTERRUPT_HANDLER 0x19, 0; reserved
INTERRUPT_HANDLER 0x1a, 0; reserved
INTERRUPT_HANDLER 0x1b, 0; reserved

INTERRUPT_HANDLER 0x1c, 0; reserved
INTERRUPT_HANDLER 0x1d, 0; reserved
INTERRUPT_HANDLER 0x1e, 0; reserved
INTERRUPT_HANDLER 0x1f, 0; reserved

INTERRUPT_HANDLER 0x20, 0; clock 时钟中断
INTERRUPT_HANDLER 0x21, 0; keyboard 键盘中断
INTERRUPT_HANDLER 0x22, 0; cascade 级联 8259
INTERRUPT_HANDLER 0x23, 0; com2 串口2
INTERRUPT_HANDLER 0x24, 0; com1 串口1
INTERRUPT_HANDLER 0x25, 0; sb16 声霸卡
INTERRUPT_HANDLER 0x26, 0; floppy 软盘
INTERRUPT_HANDLER 0x27, 0
INTERRUPT_HANDLER 0x28, 0; rtc 实时时钟
INTERRUPT_HANDLER 0x29, 0
INTERRUPT_HANDLER 0x2a, 0
INTERRUPT_HANDLER 0x2b, 0; nic 网卡
INTERRUPT_HANDLER 0x2c, 0
INTERRUPT_HANDLER 0x2d, 0
INTERRUPT_HANDLER 0x2e, 0; harddisk1 硬盘主通道
INTERRUPT_HANDLER 0x2f, 0; harddisk2 硬盘从通道

global _handler_entry_table
_handler_entry_table:
	dd interrupt_handle_0x00
	dd interrupt_handle_0x01
	dd interrupt_handle_0x02
	dd interrupt_handle_0x03
	dd interrupt_handle_0x04
	dd interrupt_handle_0x05
	dd interrupt_handle_0x06
	dd interrupt_handle_0x07
	dd interrupt_handle_0x08
	dd interrupt_handle_0x09
	dd interrupt_handle_0x0a
	dd interrupt_handle_0x0b
	dd interrupt_handle_0x0c
	dd interrupt_handle_0x0d
	dd interrupt_handle_0x0e
	dd interrupt_handle_0x0f
	
	dd interrupt_handle_0x10
	dd interrupt_handle_0x11
	dd interrupt_handle_0x12
	dd interrupt_handle_0x13
	dd interrupt_handle_0x14
	dd interrupt_handle_0x15
	dd interrupt_handle_0x16
	dd interrupt_handle_0x17
	dd interrupt_handle_0x18
	dd interrupt_handle_0x19
	dd interrupt_handle_0x1a
	dd interrupt_handle_0x1b
	dd interrupt_handle_0x1c
	dd interrupt_handle_0x1d
	dd interrupt_handle_0x1e
	dd interrupt_handle_0x1f

extern _syscall_check
extern _syscall_table
global _syscall_handler
_syscall_handler:
    push eax
    call syscall_check
    add esp, 4

    push 0xe1eccafe

    push 0x80

	push ds
	push es
	push fs
	push gs
	pusha

	push 0x80

	push ebp
	push edi
	push esi
	push edx
	push ecx
	push ebx
	
	call [_syscall_table + eax*4]
	add esp, 24
	mov dword [esp + 32], eax
	jmp interrupt_exit