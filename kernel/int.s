[bits 64]
section .text
extern interrupt_handler

; ---------------------------------------------------
; 宏：生成没有错误码的中断桩 (CPU自动补0，保证栈对齐)
; ---------------------------------------------------
%macro ISR_NOERRCODE 1
global isr%1
isr%1:
    cli
    push 0      ; 压入一个伪造的错误码
    push %1     ; 压入中断号
    jmp isr_common
%endmacro

; ---------------------------------------------------
; 宏：生成带有错误码的中断桩 (如 Page Fault)
; ---------------------------------------------------
%macro ISR_ERRCODE 1
global isr%1
isr%1:
    cli
    ; CPU 已经压入了错误码，我们只需压入中断号
    push %1
    jmp isr_common
%endmacro

; ---------------------------------------------------
; 魔法开始：自动生成 256 个中断桩！
; 注意：只有 8, 10-14, 17, 21 号异常 CPU 会自动产生错误码
; ---------------------------------------------------
ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE   8
ISR_NOERRCODE 9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_ERRCODE   17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_ERRCODE   21

; 用循环自动生成剩余的 22~255 号中断桩 (包含 32~47 的 IRQ)
%assign i 22
%rep 234
    ISR_NOERRCODE i
%assign i i+1
%endrep

; ---------------------------------------------------
; 64 位公共处理入口
; ---------------------------------------------------
isr_common:
    ; 1. 手动保存所有 64 位通用寄存器
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    cld

    ; 2. 按照 System V ABI，将栈顶指针作为第一个参数放入 RDI
    mov rdi, rsp 

    ; 3. 统一调用 C 语言的中断分发器
    call interrupt_handler

    ; 4. 恢复所有寄存器
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    ; 5. 清理中断号和错误码 (占 16 字节)
    add rsp, 16 

    ; 6. 使用 64 位的 iretq 返回！
    iretq

global isr_syscall_stub
extern syscall_handler

isr_syscall_stub:
; 1. 保护所有通用寄存器
    push rax
    push rbx
    push rcx
    push rdx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    cld

    ; 2. 将当前的栈顶指针 (rsp) 作为第一个参数 (rdi) 传给 C 函数
    ; 这样 C 函数就能通过结构体指针访问所有寄存器
    mov rdi, rsp 
    ; 3. 调用 C 语言层的分发器
    call syscall_handler

    ; 4. 恢复现场 (注意顺序与 push 相反)
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rdx
    pop rcx
    pop rbx
    pop rax  ; C 函数可能会修改栈上的 rax 值来作为返回值，这里弹出的就是修改后的值

    ; 5. 跨特权级返回用户态
    iretq
    
global isr_syscall_stub
extern syscall_handler

extern switch_to
extern timer_tick_accounting

global timer_interrupt_entry

timer_interrupt_entry:
    push rax
    push rbx
    push rcx
    push rdx
    push rbp
    push rdi
    push rsi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov rdi, rsp
    call timer_tick_accounting   ; 这里发 EOI，也可顺便记 tick

    mov rdi, rsp
    call switch_to

section .data
global isr_stub_table
isr_stub_table:
%assign i 0
%rep 256
    dq isr%+i    ; dq 表示 64 位地址 (8字节)
%assign i i+1
%endrep