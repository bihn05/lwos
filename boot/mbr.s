
[bits 16]
[org 0x7c00]

jmp init

init:

    xor ax, ax
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov sp, 0x7c00
    mov bp, sp

    mov [boot_drv], dl

    mov si, str_suc
    call print

    mov ah, 0
    mov dl, [boot_drv]
    int 0x13
    jc disk_err

    mov ah, 0x41
    mov bx, 0x55aa
    mov dl, [boot_drv]
    int 0x13
    jc .use_chs
    cmp bx, 0xaa55
    jne .use_chs
    test cl, 1
    jz .use_chs

    mov si, str_use_lbd
    call print

    mov si, dap
    mov ah, 0x42
    mov dl, [boot_drv]
    int 0x13
    jnc .load_ok

.use_chs:
    mov si, str_use_chs
    call print

    mov ax, 0
    mov es, ax
    mov bx, 0x900
    mov ah, 2       ; read 2s
    mov al, 8
    mov ch, 0       ; cylinder 0
    mov cl, 2       ; start 2s
    mov dh, 0       ; head 0
    mov dl, [boot_drv]
    int 0x13
    jc disk_err
    cmp al, 8
    jne disk_err

.load_ok:
    mov dl, [boot_drv]
    jmp 0x0:0x900

align 4
dap:
    db 0x10         ; package length
    db 0            ; resv
    dw 8            ; sects
    dw 0x900        ; offset
    dw 0            ; segment
    dq 1            ; starting LBA

print:
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

disk_err:
    mov si, str_fail
    call print
    cli
.halt:
    hlt
    jmp .halt

boot_drv: db 0x80
str_suc: db 'MBR', 0xd, 0xa, 0
str_fail: db 'FAIL', 0
str_use_chs: db 'USE CHS', 0
str_use_lbd: db 'TRY LBA', 0

times 0x1be-($-$$) db 0
partition_table:
    db 0x80             ; active booting drv
    db 0xfe, 0xff, 0xff ; chs start
    db 0x0c             ; type=FAT32, LBA
    db 0xfe, 0xff, 0xff ; chs end
    dd PART_LBA         ; lba offset (from mkfl)
    dd IMG_SECTORS - PART_LBA
    times 3 * 16 db 0

times 510-($-$$) db 0
dw 0xaa55