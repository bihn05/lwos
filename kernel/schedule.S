[bits 32]

_task_switch:
	push ebp
	mov ebp, esp

	push ebx
	push esi
	push edi

	mov eax, esp
	and eax, 0xfffff000

	mov [eax], esp

	mov eax, [ebp+8]
	mov esp, [eax]

	pop edi
	pop esi
	pop ebx
	pop ebp

	ret
global _task_switch