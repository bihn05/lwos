[bits 32]
; 中断处理函数入口 

extern _hd_table
extern _task_signal

section .text

%macro IRHD 2
; 中断句柄
ir_hd_%1:
    ; xchg bx, bx
%ifn %2
    push 0xe1eccafe
%endif
    push %1; 压入中断向量，跳转到中断入口
    jmp ir_entry
%endmacro

ir_entry:
    
    push ds
    push es
    push fs
    push gs
    pusha
    mov eax, [esp + 12 * 4]
    push eax
    call [_hd_table + eax * 4]

global ir_exit
ir_exit:
    add esp, 4
;    call _task_signal
    popa
    pop gs
    pop fs
    pop es
    pop ds

    add esp, 8
    iret

IRHD 0x00, 0; divide by zero
IRHD 0x01, 0; debug
IRHD 0x02, 0; non maskable interrupt
IRHD 0x03, 0; breakpoint
IRHD 0x04, 0; overflow
IRHD 0x05, 0; bound range exceeded
IRHD 0x06, 0; invalid opcode
IRHD 0x07, 0; device not avilable
IRHD 0x08, 1; double fault
IRHD 0x09, 0; coprocessor segment overrun
IRHD 0x0a, 1; invalid TSS
IRHD 0x0b, 1; segment not present
IRHD 0x0c, 1; stack segment fault
IRHD 0x0d, 1; general protection fault
IRHD 0x0e, 1; page fault
IRHD 0x0f, 0; reserved
IRHD 0x10, 0; x87 floating point exception
IRHD 0x11, 1; alignment check
IRHD 0x12, 0; machine check
IRHD 0x13, 0; SIMD Floating - Point Exception
IRHD 0x14, 0; Virtualization Exception
IRHD 0x15, 1; Control Protection Exception
IRHD 0x16, 0; reserved
IRHD 0x17, 0; reserved
IRHD 0x18, 0; reserved
IRHD 0x19, 0; reserved
IRHD 0x1a, 0; reserved
IRHD 0x1b, 0; reserved
IRHD 0x1c, 0; reserved
IRHD 0x1d, 0; reserved
IRHD 0x1e, 0; reserved
IRHD 0x1f, 0; reserved
IRHD 0x20, 0; clock 时钟中断
IRHD 0x21, 0; keyboard 键盘中断
IRHD 0x22, 0; cascade 级联 8259
IRHD 0x23, 0; com2 串口2
IRHD 0x24, 0; com1 串口1
IRHD 0x25, 0; sb16 声霸卡
IRHD 0x26, 0; floppy 软盘
IRHD 0x27, 0
IRHD 0x28, 0; rtc 实时时钟
IRHD 0x29, 0
IRHD 0x2a, 0
IRHD 0x2b, 0; nic 网卡
IRHD 0x2c, 0
IRHD 0x2d, 0
IRHD 0x2e, 0; harddisk1 硬盘主通道
IRHD 0x2f, 0; harddisk2 硬盘从通道

global _hd_entry_table
_hd_entry_table:
    dd ir_hd_0x00
    dd ir_hd_0x01
    dd ir_hd_0x02
    dd ir_hd_0x03
    dd ir_hd_0x04
    dd ir_hd_0x05
    dd ir_hd_0x06
    dd ir_hd_0x07
    dd ir_hd_0x08
    dd ir_hd_0x09
    dd ir_hd_0x0a
    dd ir_hd_0x0b
    dd ir_hd_0x0c
    dd ir_hd_0x0d
    dd ir_hd_0x0e
    dd ir_hd_0x0f
    dd ir_hd_0x10
    dd ir_hd_0x11
    dd ir_hd_0x12
    dd ir_hd_0x13
    dd ir_hd_0x14
    dd ir_hd_0x15
    dd ir_hd_0x16
    dd ir_hd_0x17
    dd ir_hd_0x18
    dd ir_hd_0x19
    dd ir_hd_0x1a
    dd ir_hd_0x1b
    dd ir_hd_0x1c
    dd ir_hd_0x1d
    dd ir_hd_0x1e
    dd ir_hd_0x1f
    dd ir_hd_0x20
    dd ir_hd_0x21
    dd ir_hd_0x22
    dd ir_hd_0x23
    dd ir_hd_0x24
    dd ir_hd_0x25
    dd ir_hd_0x26
    dd ir_hd_0x27
    dd ir_hd_0x28
    dd ir_hd_0x29
    dd ir_hd_0x2a
    dd ir_hd_0x2b
    dd ir_hd_0x2c
    dd ir_hd_0x2d
    dd ir_hd_0x2e
    dd ir_hd_0x2f