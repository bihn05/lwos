; 文件安排表，每项都是一个fat_t结构体

db '/', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
dd 0
db 0, 0, 0
db 0x88
dd 0xffffffff ; cluster start
dd 0xffffffff ; parent
dd 1 ; first child
dd 0xffffffff ; next sibling
dd 0
dq 0

db 'LWLDR', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
dd 48108
db 'BIN'
db 0x83
dd 2
dd 0
dd 0xffffffff
dd 2
dd 0
dq 1759759641

db 'TEST', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
dd 230
db 'TXT'
db 0x83
dd 27
dd 0
dd 0xffffffff
dd 3
dd 0
dq 1759759641

db 'TEST', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
dd 13220
db 'ELF'
db 0x83
dd 28
dd 0
dd 0xffffffff
dd 0xffffffff
dd 0
dq 1759759641
