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

global timer_schedule
extern do_timer_tick

timer_schedule:
	pusha
	
	push ds
	push es
	push fs
	push gs

	mov ax, 0x10
	mov ds, ax
	mov es, ax

	mov al, 0x20
	out 0x20, al

	call do_timer_tick

	pop gs
	pop fs
	pop es
	pop ds

	popa

	iret