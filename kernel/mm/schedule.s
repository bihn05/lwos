extern current_task
extern schedule

; void context_switch(task_t *next);
global context_switch
context_switch:
	mov eax, [current_task]

	push ebp
	mov ebp, esp

	pushf
	pusha
	mov [eax], esp

	mov edx, [ebp+8]
	mov [current_task], edx
	mov esp, [edx]

	popa
	popf
	pop ebp
	ret

global kernel_thread_entry
kernel_thread_entry:
	sti
	call ebx
.die:
	hlt
	jmp .die

global switch_task
; void switch_task(task_t* prev, task_t* next);
switch_task:

	; get prmt from c call
	mov eax, [esp + 4]
	mov edx, [esp + 8]

	; save used
	push ebp
	push ebx
	push esi
	push edi

	mov [eax], esp
	mov esp, [edx]

	mov eax, [edx + 4]

	pop edi
	pop esi
	pop ebx
	pop ebp

	ret

global switch_to
switch_to:
    ; 此时栈里是: [返回地址] [prev指针] [next指针]
    
    ; 1. 保存当前任务 (prev) 的环境
    push esi
    push edi
    push ebx
    push ebp

    mov eax, [esp + 20]     ; 获取 prev 指针 (4个push占16字节 + 返回地址4字节)
    mov [eax], esp          ; 把当前的 esp 存入 prev->esp (结构体第一个成员)

    ; 2. 切换到下一个任务 (next) 的环境
    mov eax, [esp + 24]     ; 获取 next 指针
    mov esp, [eax]          ; 把 next->esp 强行赋予硬件 ESP 寄存器！(栈在此刻被彻底偷换)

    ; (可选) 如果每个进程有独立的页表，在这里加载 next 的 CR3
    ; mov eax, [eax + 20]   ; 假设 page_dir_phys 偏移是 20
    ; mov cr3, eax

    ; 3. 恢复 next 任务的环境
    pop ebp
    pop ebx
    pop edi
    pop esi

    ret