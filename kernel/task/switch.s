[bits 64]

extern kernel_thread_entry
global kernel_thread_stub

; 新线程第一次被调度时，switch_to 会 ret 到这里
kernel_thread_stub:
    ; 此时，rbx 里装着目标函数指针，r12 里装着函数参数 (我们在 thread_create 里伪造的)
    mov rdi, rbx  ; 参数 1: function
    mov rsi, r12  ; 参数 2: func_arg
    
    call kernel_thread_entry ; 跳转到 C 语言入口执行
    
.die:
    hlt           ; 如果线程函数意外 return 了，直接停机防止跑飞
    jmp .die

global switch_to

; void switch_to(task_struct_t* current, task_struct_t* next);
; 根据 System V ABI，参数 1 (current) 在 RDI，参数 2 (next) 在 RSI
switch_to:
    ; 1. 保存当前线程的执行上下文 (只需保存 Callee-saved 寄存器)
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15

    ; 2. 核心：保存当前栈顶指针到 current->kernel_stack
    ; 由于 kernel_stack 在 task_struct 的偏移量为 0，[rdi] 就是它的地址
    mov [rdi], rsp

    ; ---------------------------------------------------------
    ; 这一步跨越了线程的边界，CPU 的栈已经被替换为新线程的栈！
    ; ---------------------------------------------------------

    ; 3. 核心：从 next->kernel_stack 恢复新线程的栈顶指针
    mov rsp, [rsi]

    ; 4. 恢复新线程的执行上下文
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp

    ; 5. 返回到新线程上次调用 switch_to 的地方 (或者新线程的入口)
    ret