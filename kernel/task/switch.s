[bits 64]

extern kernel_thread_bootstrap
; extern thread_exit

global kernel_thread_stub

kernel_thread_stub:
    cld
    call kernel_thread_bootstrap
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