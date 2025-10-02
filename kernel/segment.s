[bits 32]
section .text

global _load_segments
_load_segments:
	mov ax, 0x10
;	jmp $
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

	jmp 0x08:.reload.cs
.reload.cs:
	ret