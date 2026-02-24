[bits 64]
global load_segments

load_segments:
    ; 1. 刷新数据段寄存器
    mov ax, 0x10      ; 内核数据段选择子 (GDT 索引 2)
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax

    ; 2. 刷新代码段 (CS)
    ; 构造栈帧：[SS (仅在特权级切换时需要)] -> [RSP] -> RFLAGS -> CS -> RIP
    ; 在同特权级刷新时，只需要 PUSH CS 和 PUSH RIP
    push 0x08         ; 新的 CS 选择子
    lea rax, [rel .flush]
    push rax          ; 压入返回地址 (RIP)
    
    ; 使用 o64 前缀确保 NASM 生成 64 位的远返回指令
    o64 retf          

.flush:
    ret