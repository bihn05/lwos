/*
 * ABI.BIN: 服务表 + 初始化入口
 * 这是一个库映像, 没有主循环也没有自己的栈, 用调用者的栈
 * 函数声明来自 dev/ 的头文件; 这里只 extern 链接脚本符号
 */

#include "abi.h"
#include "stdint.h"
#include "text.h"
#include "io.h"

extern char __bss_start[], __bss_end[];

int abi_init(void);

const PVOID lw_abi[LW_SLOT_COUNT] __attribute__((section(".abi"), used, aligned(16))) = {
    [LW_SLOT_MAGIC]     = (PVOID)LW_ABI_MAGIC,
    [LW_SLOT_ENTRY]     = (PVOID)abi_init,
    [LW_SLOT_BSS_START] = (PVOID)__bss_start,
    [LW_SLOT_BSS_END]   = (PVOID)__bss_end,
    [LW_SLOT_STACK_TOP] = 0,    /* 库映像, 无自己的栈 */

    [LW_SLOT_CONSOLE_PUTC]      = (PVOID)putc,
    [LW_SLOT_CONSOLE_PUTS]      = (PVOID)puts,
    [LW_SLOT_CONSOLE_PUTCA]     = (PVOID)putca,
    [LW_SLOT_CONSOLE_PUT_BYTE]  = (PVOID)put_byte,
    [LW_SLOT_CONSOLE_PUT_WORD]  = (PVOID)put_word,
    [LW_SLOT_CONSOLE_PUT_DWORD] = (PVOID)put_dword,
    [LW_SLOT_CONSOLE_PUT_QWORD] = (PVOID)put_qword,
    [LW_SLOT_CONSOLE_CLEAR]     = (PVOID)screen_clear,
    [LW_SLOT_CONSOLE_DUMP128]   = (PVOID)dump128,

    [LW_SLOT_IO_OUTB]           = (PVOID)outb,
};

/* 自己清自己的 bss, 调用方不需要知道 ABI.BIN 的内存布局 */
int abi_init(void) {
    for (char *p = __bss_start; p < __bss_end; p++) {
        *p = 0;
    }
    return 0;
}
