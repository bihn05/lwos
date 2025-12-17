global flush_tlb
flush_tlb:
	push ebp
	mov ebp, esp
	mov eax, [esp+8]
	invlpg [eax]
	pop ebp
	ret