; switch.s

[bits 64]

extern kernel_thread_bootstrap
extern thread_exit

global kernel_thread_stub

kernel_thread_stub:
    cld
    call kernel_thread_bootstrap
    call thread_exit
.hang:
    hlt
    jmp .hang

extern schedule
global switch_to

; intr_frame_t* switch_to(intr_frame_t* current_frame)
; rdi = 当前线程被中断时的 frame 栈顶
switch_to:
    ; rdi = current_frame
    call schedule          ; rax = next_frame
    mov rsp, rax

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rsi
    pop rdi
    pop rbp
    pop rdx
    pop rcx
    pop rbx
    pop rax

    xchg bx, bx
    iretq

global thread_exit_switch

; void thread_exit_switch(intr_frame_t* next_frame)
; rdi = next_frame
thread_exit_switch:
    mov rsp, rdi

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rsi
    pop rdi
    pop rbp
    pop rdx
    pop rcx
    pop rbx
    pop rax
    iretq

global context_switch

; void context_switch(task_struct_t* prev, task_struct_t* next)
; rdi = prev
; rsi = next
context_switch:
    ; 保存被调用者保存寄存器
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15

    ; 保存当前线程普通上下文栈顶
    mov [rdi], rsp

    ; 切到 next 的普通上下文栈
    mov rsp, [rsi]

    ; 恢复 next 的被调用者保存寄存器
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp

    ret

global save_current_frame_and_switch

%define IF_R15     0x00
%define IF_R14     0x08
%define IF_R13     0x10
%define IF_R12     0x18
%define IF_R11     0x20
%define IF_R10     0x28
%define IF_R9      0x30
%define IF_R8      0x38
%define IF_RSI     0x40
%define IF_RDI     0x48
%define IF_RBP     0x50
%define IF_RDX     0x58
%define IF_RCX     0x60
%define IF_RBX     0x68
%define IF_RAX     0x70
%define IF_RIP     0x78
%define IF_CS      0x80
%define IF_RFLAGS  0x88
%define IF_RSP     0x90
%define IF_SS      0x98

; void save_current_frame_and_switch(intr_frame_t* prev_frame, intr_frame_t* next_frame)
; rdi = prev_frame
; rsi = next_frame
save_current_frame_and_switch:
    ; ---------- 保存当前“普通执行流”到 prev_frame ----------
    mov [rdi + IF_R15], r15
    mov [rdi + IF_R14], r14
    mov [rdi + IF_R13], r13
    mov [rdi + IF_R12], r12
    mov [rdi + IF_RBP], rbp
    mov [rdi + IF_RBX], rbx

    ; caller-saved 这里不要求精确保留，置 0 即可
    xor rax, rax
    mov [rdi + IF_R11], rax
    mov [rdi + IF_R10], rax
    mov [rdi + IF_R9 ], rax
    mov [rdi + IF_R8 ], rax
    mov [rdi + IF_RSI], rax
    mov [rdi + IF_RDI], rax
    mov [rdi + IF_RDX], rax
    mov [rdi + IF_RCX], rax
    mov [rdi + IF_RAX], rax

    ; 以后恢复这个线程时，先回到 .resume，然后 ret 回 C 调用点
    lea rax, [rel .resume]
    mov [rdi + IF_RIP], rax

    mov qword [rdi + IF_CS], 0x08

    pushfq
    pop rax
    mov [rdi + IF_RFLAGS], rax

    ; 当前 rsp 顶端就是本函数返回地址，恢复后 .resume -> ret 正好回到 C 调用点
    mov rax, rsp
    mov [rdi + IF_RSP], rax

    mov qword [rdi + IF_SS], 0x10

    ; ---------- 切到 next_frame ----------
    mov rsp, rsi

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rsi
    pop rdi
    pop rbp
    pop rdx
    pop rcx
    pop rbx
    pop rax
    iretq

.resume:
    ret