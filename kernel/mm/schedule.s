; void context_switch(uint32_t* old_esp, uint32_t new_esp);
global _context_switch
_context_switch:
	mov eax, [esp + 4]
	mov edx, [esp + 8]

	mov [eax], esp

	mov esp, edx

	popa

	iretd
