
[bits 32]
section .text

extern idt_pointer
extern interrupt_handler
extern irq_handler
;extern int_page_fault_handler

global isr0
global isr1
global isr2
global isr3
global isr4
global isr5
global isr6
global isr7
global isr8
global isr9
global isr10
global isr11
global isr12
global isr13
global isr14
global isr15
global isr16
global isr17
global isr18
global isr19
global isr20
global isr21

%macro IRQ 2
global irq%1
irq%1:
	cli
	push dword 0
	push dword %2
	jmp irq_common
%endmacro

; IRQ �ж� (32-47)
IRQ 0, 32   ; ��ʱ���ж�
IRQ 1, 33   ; �����ж�
IRQ 2, 34   ; �����ж�
IRQ 3, 35   ; COM2
IRQ 4, 36   ; COM1
IRQ 5, 37   ; LPT2
IRQ 6, 38   ; ����
IRQ 7, 39   ; LPT1
IRQ 8, 40   ; CMOSʵʱ��
IRQ 9, 41   ; �����ж�
IRQ 10, 42  ; �����ж�
IRQ 11, 43  ; �����ж�
IRQ 12, 44  ; PS2���
IRQ 13, 45  ; Э������
IRQ 14, 46  ; ��Ӳ��
IRQ 15, 47  ; ��Ӳ��

isr0:
	cli
	push dword 0
	push dword 0
	jmp isr_common
isr1:
	cli
	push dword 0
	push dword 1
	jmp isr_common
isr2:
	cli
	push dword 0
	push dword 2
	jmp isr_common
isr3:
	cli
	push dword 0
	push dword 3
	jmp isr_common
isr4:
	cli
	push dword 0
	push dword 4
	jmp isr_common
isr5:
	cli
	push dword 0
	push dword 5
	jmp isr_common
isr6:
	cli
	push dword 0
	push dword 6
	jmp isr_common
isr7:
	cli
	push dword 0
	push dword 7
	jmp isr_common
isr8:
	cli
	push dword 8
	jmp isr_common
isr9:
	cli
	push dword 0
	push dword 9
	jmp isr_common
isr10:
	cli
	push dword 10
	jmp isr_common
isr11:
	cli
	push dword 11
	jmp isr_common
isr12:
	cli
	push dword 12
	jmp isr_common
isr13:
	cli
	push dword 13
	jmp isr_common
isr14:
	cli
	push dword 15
	pushad

	push ds
	push es
	push fs
	push gs

	mov eax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	push esp
;	call int_page_fault_handler
	
	add esp, 4

	pop gs
	pop fs
	pop es
	pop ds

	popad

	add esp, 8

	sti
	iret
isr15:
	cli
	push dword 0
	push dword 16
	jmp isr_common
isr16:
	cli
	push dword 0
	push dword 16
	jmp isr_common
isr17:
	cli
	push dword 0
	push dword 17
	jmp isr_common
isr18:
	cli
	push dword 0
	push dword 18
	jmp isr_common
isr19:
	cli
	push dword 0
	push dword 19
	jmp isr_common
isr20:
	cli
	push dword 0
	push dword 19
	jmp isr_common
isr21:
	cli
	push dword 19
	jmp isr_common

isr_common:
	pushad

	push ds
	push es
	push fs
	push gs

	mov eax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	push esp
	call interrupt_handler
	
	add esp, 4

	pop gs
	pop fs
	pop es
	pop ds

	popad

	add esp, 8

	sti
	iret

irq_common:
	pushad
	push ds
	push es
	push fs
	push gs

	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	push esp
	call irq_handler
	add esp, 4

	pop gs
	pop fs
	pop es
	pop ds
	popad

	add esp, 8

	sti
	iret

global idt_flush
idt_flush:
	push ebp
	mov ebp, esp
	lidt [idt_pointer]
	pop ebp
	ret

global d_test_int0
d_test_int0:
	push ebp
	mov ebp, esp
	xor eax, eax
	div eax
	pop ebp
	ret

global set_interrupt_mask
set_interrupt_mask:
	push ebp
	mov ebp, esp
	sub esp, 8
	mov al, [esp+0x10]
	and al, 0x08
	mov bl, 0x08
	cmp al, bl
	jne .n1
	mov byte [esp+4], 0x21
	jmp .n2
.n1:
	mov byte [esp+4], 0xa1
	sub byte [esp+0x10], 8
.n2:
	xor dx, dx
	mov dl, [esp+4]
	in al, dx
	mov [esp], al
	mov al, [esp+0x14]
	mov bl, 1
	cmp al, bl
	jne .n3
	mov cl, [esp+0x10]
	mov al, 1
	shl al, cl
	or byte [esp], al
	jmp .done
.n3:
	mov cl, [esp+0x10]
	mov al, 1
	shl al, cl
	xor al, 0xff
	and byte [esp], al
.done:
	xor dx, dx
	mov dl, [esp+4]
	mov al, [esp]
	out dx, al
	add esp, 8
	pop ebp
	ret

global pic_init
pic_init:
	push ebp
	mov ebp, esp
	sub esp, 8
	in al, 0x21
	mov [esp], al; master mask
	in al, 0xa1
	mov [esp+4], al ; slave mask

	mov al, 0xff
	out 0x21, al
	out 0xa1, al

	mov al, 0x11
	out 0x20, al
	call io_delay
	mov al, 0x20
	out 0x21, al
	call io_delay
	mov al, 0x04
	out 0x21, al
	call io_delay
	mov al, 0x01
	out 0x21, al
	call io_delay

	mov al, 0x11
	out 0xa0, al
	call io_delay
	mov al, 0x28
	out 0xa1, al
	call io_delay
	mov al, 0x02
	out 0xa1, al
	call io_delay
	mov al, 0x01
	out 0xa1, al
	call io_delay

	mov al, [esp]
	and al, 0xfc
	out 0x21, al
	mov al, [esp+4]
	out 0xa1, al

	add esp, 8
	pop ebp
	ret
io_delay:
	nop
	nop
	nop
	nop
	ret
	
extern keyboard_handler
global keyboard_hd
keyboard_hd:
	cli
	call keyboard_handler
	sti
	iret
extern timer_handler
global timer_hd
timer_hd:
	cli
	call timer_handler
	sti
	iret

global test
test:
	push ebp
	mov ebp, esp
	mov eax, [esp+0xc]
	jmp $

%macro UHINT 1
global unhandle_int%1
unhandle_int%1:
	cli
	mov eax, %1
	mov ebx, 0xcafebabe
	jmp $
	iret
%endmacro

UHINT 48
UHINT 49
UHINT 50
UHINT 51
UHINT 52
UHINT 53
UHINT 54
UHINT 55
UHINT 56
UHINT 57
UHINT 58
UHINT 59
UHINT 60
UHINT 61
UHINT 62
UHINT 63
UHINT 64
UHINT 65
UHINT 66
UHINT 67
UHINT 68
UHINT 69
UHINT 70
UHINT 71
UHINT 72
UHINT 73
UHINT 74
UHINT 75
UHINT 76
UHINT 77
UHINT 78
UHINT 79
UHINT 80
UHINT 81
UHINT 82
UHINT 83
UHINT 84
UHINT 85
UHINT 86
UHINT 87
UHINT 88
UHINT 89
UHINT 90
UHINT 91
UHINT 92
UHINT 93
UHINT 94
UHINT 95
UHINT 96
UHINT 97
UHINT 98
UHINT 99
UHINT 100
UHINT 101
UHINT 102
UHINT 103
UHINT 104
UHINT 105
UHINT 106
UHINT 107
UHINT 108
UHINT 109
UHINT 110
UHINT 111
UHINT 112
UHINT 113
UHINT 114
UHINT 115
UHINT 116
UHINT 117
UHINT 118
UHINT 119
UHINT 120
UHINT 121
UHINT 122
UHINT 123
UHINT 124
UHINT 125
UHINT 126
UHINT 127
UHINT 128
UHINT 129
UHINT 130
UHINT 131
UHINT 132
UHINT 133
UHINT 134
UHINT 135
UHINT 136
UHINT 137
UHINT 138
UHINT 139
UHINT 140
UHINT 141
UHINT 142
UHINT 143
UHINT 144
UHINT 145
UHINT 146
UHINT 147
UHINT 148
UHINT 149
UHINT 150
UHINT 151
UHINT 152
UHINT 153
UHINT 154
UHINT 155
UHINT 156
UHINT 157
UHINT 158
UHINT 159
UHINT 160
UHINT 161
UHINT 162
UHINT 163
UHINT 164
UHINT 165
UHINT 166
UHINT 167
UHINT 168
UHINT 169
UHINT 170
UHINT 171
UHINT 172
UHINT 173
UHINT 174
UHINT 175
UHINT 176
UHINT 177
UHINT 178
UHINT 179
UHINT 180
UHINT 181
UHINT 182
UHINT 183
UHINT 184
UHINT 185
UHINT 186
UHINT 187
UHINT 188
UHINT 189
UHINT 190
UHINT 191
UHINT 192
UHINT 193
UHINT 194
UHINT 195
UHINT 196
UHINT 197
UHINT 198
UHINT 199
UHINT 200
UHINT 201
UHINT 202
UHINT 203
UHINT 204
UHINT 205
UHINT 206
UHINT 207
UHINT 208
UHINT 209
UHINT 210
UHINT 211
UHINT 212
UHINT 213
UHINT 214
UHINT 215
UHINT 216
UHINT 217
UHINT 218
UHINT 219
UHINT 220
UHINT 221
UHINT 222
UHINT 223
UHINT 224
UHINT 225
UHINT 226
UHINT 227
UHINT 228
UHINT 229
UHINT 230
UHINT 231
UHINT 232
UHINT 233
UHINT 234
UHINT 235
UHINT 236
UHINT 237
UHINT 238
UHINT 239
UHINT 240
UHINT 241
UHINT 242
UHINT 243
UHINT 244
UHINT 245
UHINT 246
UHINT 247
UHINT 248
UHINT 249
UHINT 250
UHINT 251
UHINT 252
UHINT 253
UHINT 254
UHINT 255
UHINT 256