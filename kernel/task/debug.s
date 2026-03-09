[bits 64]
global test_serial

test_serial:
    mov al, 'S'      ; 'S' 的 ASCII 码 0x53
    out 0xE9, al     ; 输出到端口 0xE9
    ret