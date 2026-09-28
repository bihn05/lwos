[org 0x200000]
[bits 32]

push dword str_test 
call [0x100044]
add esp, 4

ret

str_test: db "HELLO FROM TESTTTTTT", 13, 10, 0