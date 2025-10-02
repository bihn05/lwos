[org 0x7c00]
jmp init

db 0 ; 跳转码只占3字节
fs_name:				db "EXFAT   "
times 53 db 0
; 关键参数（需动态计算或初始化）
partition_offset:       dq 0            ; 分区起始LBA（通常为0，若为分区表中的分区则需调整）
volume_length:          dq 0            ; 卷总扇区数（需动态填充）
fat_offset:             dd 0            ; FAT表起始扇区号（相对分区起始）
fat_length:             dd 0            ; FAT表占用的扇区数
cluster_heap_offset:    dd 0            ; 簇堆起始扇区号
cluster_count:          dd 0            ; 总簇数
root_directory_cluster: dd 2            ; 根目录起始簇号（exFAT固定为2）
volume_serial_number:   dd 0x12345678   ; 卷序列号（可随机生成）
fs_revision:            dw 0x0100       ; 文件系统版本（1.0）
fs_flags:               dw 0            ; 标志位（通常为0）
bps_shift:              db 9            ; 每扇区字节数移位值（2^9=512字节）
spc_shift:              db 8            ; 每簇扇区数移位值（2^8=256扇区/簇）
number_of_fats:         db 1            ; FAT表数量（exFAT固定为1）
drive_select:           db 0x80         ; 驱动器号（0x80表示第1个硬盘）
percent_in_use:         db 0            ; 已用簇百分比（启动时为0）
reserved:               times 7 db 0    ; 保留字段（7字节）

init:
	mov ax, 0
	mov ss, ax
	mov ds, ax
	mov sp, 0x7c00
	mov bp, sp

	mov si, str_1
	call print

	mov ah, 0
	mov dl, 0x80
	int 0x13
	jc disk_err

	mov ax, 0x0000
	mov es, ax
	mov ah, 0x02
	mov al, 0x08 ; 8*512=4KB
	mov ch, 0x00 ; 柱面
	mov cl, 0x02 ; 扇区
	mov dh, 0x00 ; 磁头
	mov dl, 0x80 ; 设备
	mov bx, 0x0900 ; [es:bx]
	int 0x13
	jc disk_err

	jmp 0x900

print:
.L1:
	mov al, [si]
	inc si
	cmp al, 0
	je .done
	mov ah, 0x0e
	int 0x10
	jmp .L1
.done:
	ret

disk_err:
	mov si, str_2
	call print
	hlt

str_1: db 'MBR OK', 0
str_2: db 'MEDIA ERR', 0

times 510-($-$$) db 0
dw 0xAA55