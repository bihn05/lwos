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

; switch_to(task_struct_t* current, task_struct_t* next)
switch_to:
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15

    ; 保存 current 栈
    mov [rdi], rsp

    ; --- 用户态进程 CR3 切换 ---
    mov rax, [rsi + 0x30]   ; 假设 pml4_dir 偏移是 0x30
    test rax, rax
    jz .skip_cr3
    mov cr3, rax
.skip_cr3:

    ; 切换到 next 栈
    mov rsp, [rsi]

    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp

    ret