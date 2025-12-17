extern current_task
extern schedule

; void context_switch(task_t *next);
global context_switch
context_switch:
	pusha
	pushf

	mov eax, [current_task]
	mov [eax], esp

	; switch stack

	mov edx, [esp + 40]
	mov [current_task], edx
	
	mov esp, [edx]

	mov ecx, [edx + 4]
	mov ebx, cr3
	cmp ebx, ecx
	je .skip_cr3
	mov cr3, ecx
.skip_cr3:

	popf
	popa

	ret

global timer_schedule
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

	call schedule

	pop gs
	pop fs
	pop es
	pop ds

	popa

	iret