
[bits 16]
section .stage2.entry
global stage2_entry

VBE_INFO_BLOCK      equ 0x9100
FAT_SCRATCH         equ 0x9100

stage2_entry:

    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x900
    mov [BOOT_DRIVE], dl

    call kernel_load
    jmp $

; =================================
; detailed dap buffer structures
;    +0h  BYTE  length of the buffer = 10h
;    +1h  BYTE  reserved
;    +2h  WORD  sectors count
;    +4h  WORD  buffer address 0000xxxxh
;    +6h  WORD  buffer segment xxxx0000h
;    +8h QWORD  start lba

; =================================
; detailed fat32 structures
;    +0h    3B  jump instructions
;    +3h    8B  OEM
;    +bh   80B  BPB and extended BPB
;   +5bh  419B  codes
;  +1feh  WORD  0xaa55
; ---------------------------------
; BPB inside
;    +bh  WORD  bytes per sector
;    +dh  BYTE  sectors per cluster
;    +eh  WORD  sectors count from boot to 1st FAT
;               defaultly 32(20h)
;    

kernel_load:
    mov byte [dap_buffer], 0x10
    mov byte [dap_buffer + 1], 0
    mov word [dap_buffer + 2], 1              ; 1 s
    mov word [dap_buffer + 4], FAT_SCRATCH    ; to fat scr
    mov word [dap_buffer + 6], 0              ; seg0
    mov dword [dap_buffer + 8], PART_LBA      ; lba start
    mov dword [dap_buffer + 12], 0            ; 0
    call read_sector
    jc .ioerr

    cmp word [FAT_SCRATCH + 0x1fe], 0xaa55
    jne .nofs

    cmp byte [FAT_SCRATCH + 0x0D], 1
    jne .nofs

    ; 
    movzx eax, word [FAT_SCRATCH + 0x0e] ; word -> dword
    add eax, PART_LBA
    mov [FAT_START_LBA], eax

.ioerr:
    mov si, str_ioerr
    call print16
    jmp .halted
.halted:
    hlt
    jmp .halted

read_sector:
    mov bx, 3 ； 3 times try
.retry
    pushad
    mov si, dap_buffer
    mov ah, 0x42
    mov dl, [BOOT_DRIVE]
    int 0x13
    popad
    jnc .ok
    dec bx
    jz .fail
    push ax
    xor ax, ax
    mov dl, [BOOT_DRIVE]
    int 0x13
    pop ax
    jmp .retry
.ok:
    clc
    ret
.fail:
    stc
    ret

print16:
.L1:
    mov al, [si]
    inc si
    test al, al
    jz .done
    mov ah, 0x0e
    int 0x10
    jmp .L1
.done:
    ret

DEBUG_NAME: db "DEBUG      "
FAT_START_LBA   equ 0x8A10
DATA_START_LBA  equ 0x8A14
cur_cluster     equ 0x8A18

dap_buffer: