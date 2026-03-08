[bits 64]

global start

extern kernel_init
section .text
start:
    mov rsp, 0xFFFFFFFF80090000
    mov rbp, rsp

    push 0
    popf

    call kernel_init
    jmp $