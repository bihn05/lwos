#ifndef _LW_ABI_H
#define _LW_ABI_H

/*
 * LWOS ABI 共享接口
 *
 * 提供方: ABI.BIN (abi/abi.c), 固定加载在 LW_ABI_BASE, 表在映像最开头
 * 调用方: 不链接提供方的任何符号, 只通过 lw_abi_base 查表调用
 *
 * slot 0x00-0x0F 同时是所有可加载映像的通用头 (MONITOR.BIN 也有),
 * loader 的 P 命令靠它校验、清 bss、换栈、跳转
 *
 * 唯一规则: slot 只追加。不改号、不复用、不删除。
 *           废弃的 slot 留空 (NULL), 编号永久占用。
 */

#include "stdint.h"

#define LW_ABI_MAGIC 0x4241574cu    /* "LWAB" */
#define LW_ABI_BASE  0x00100000u

enum {
    // description of the image
    LW_SLOT_MAGIC               = 0x00,
    LW_SLOT_ENTRY               = 0x01,     /* 程序: 入口; ABI.BIN: 初始化函数 */
    LW_SLOT_BSS_START           = 0x02,
    LW_SLOT_BSS_END             = 0x03,
    LW_SLOT_STACK_TOP           = 0x04,

    // console output
    LW_SLOT_CONSOLE_PUTC        = 0x10,
    LW_SLOT_CONSOLE_PUTS        = 0x11,
    LW_SLOT_CONSOLE_PUTCA       = 0x12,
    LW_SLOT_CONSOLE_PUT_BYTE    = 0x13,
    LW_SLOT_CONSOLE_PUT_WORD    = 0x14,
    LW_SLOT_CONSOLE_PUT_DWORD   = 0x15,
    LW_SLOT_CONSOLE_PUT_QWORD   = 0x16,
    LW_SLOT_CONSOLE_CLEAR       = 0x17,
    LW_SLOT_CONSOLE_DUMP128     = 0x18,
    LW_SLOT_CONSOLE_PUT_DEC32   = 0x19,
    LW_SLOT_CONSOLE_GETK        = 0x1a,
    LW_SLOT_CONSOLE_GETC        = 0x1b,
    LW_SLOT_CONSOLE_GETP        = 0x1c,

    // devices
    // not out dx, al because this way may
    // create some JOURNEY
    LW_SLOT_IO_OUTB             = 0x20,
    LW_SLOT_IO_OUTW             = 0x21,
    LW_SLOT_IO_OUTL             = 0x22,
    LW_SLOT_IO_INB              = 0x23,
    LW_SLOT_IO_INW              = 0x24,
    LW_SLOT_IO_INL              = 0x25,
    LW_SLOT_IO_KBD_PROBE        = 0x26,
    LW_SLOT_IO_KBD_ENABLE       = 0x27,

    LW_SLOT_COUNT
};

/*
 * 调用方用法:
 *   PVOID *lw_abi_base;                  // 每个调用方程序定义一次
 *   if (lw_abi_attach((PVOID)LW_ABI_BASE) != 0) ...   // 校验 + 初始化
 *   lw_puts("hello\n\r");
 */
extern PVOID *lw_abi_base;

#define LW_ABI_VALID()      ((DWORD)lw_abi_base[LW_SLOT_MAGIC] == LW_ABI_MAGIC)
#define LW_CALL(slot, type) ((type)lw_abi_base[slot])

#define lw_putc         LW_CALL(LW_SLOT_CONSOLE_PUTC,      void (*)(char))
#define lw_puts         LW_CALL(LW_SLOT_CONSOLE_PUTS,      void (*)(char *))
#define lw_putca        LW_CALL(LW_SLOT_CONSOLE_PUTCA,     void (*)(char))
#define lw_put_byte     LW_CALL(LW_SLOT_CONSOLE_PUT_BYTE,  void (*)(BYTE))
#define lw_put_word     LW_CALL(LW_SLOT_CONSOLE_PUT_WORD,  void (*)(WORD))
#define lw_put_dword    LW_CALL(LW_SLOT_CONSOLE_PUT_DWORD, void (*)(DWORD))
#define lw_put_qword    LW_CALL(LW_SLOT_CONSOLE_PUT_QWORD, void (*)(QWORD))
#define lw_screen_clear LW_CALL(LW_SLOT_CONSOLE_CLEAR,     void (*)(void))
#define lw_dump128      LW_CALL(LW_SLOT_CONSOLE_DUMP128,   void (*)(PVOID))
#define lw_getk         LW_CALL(LW_SLOT_CONSOLE_GETK,      int (*)(void))
#define lw_getc         LW_CALL(LW_SLOT_CONSOLE_GETC,      char (*)(void))
#define lw_getp         LW_CALL(LW_SLOT_CONSOLE_GETP,      int (*)(void))

#define lw_outb         LW_CALL(LW_SLOT_IO_OUTB,           void (*)(WORD, BYTE))
#define lw_outw         LW_CALL(LW_SLOT_IO_OUTW,           void (*)(WORD, WORD))
#define lw_outl         LW_CALL(LW_SLOT_IO_OUTL,           void (*)(WORD, DWORD))
#define lw_inb          LW_CALL(LW_SLOT_IO_INB,            BYTE (*)(WORD))
#define lw_inw          LW_CALL(LW_SLOT_IO_INW,            WORD (*)(WORD))
#define lw_inl          LW_CALL(LW_SLOT_IO_INL,            DWORD (*)(WORD))
#define lw_kbd_probe    LW_CALL(LW_SLOT_IO_KBD_PROBE,      void (*)(void))
#define lw_kbd_enable   LW_CALL(LW_SLOT_IO_KBD_ENABLE,     void (*)(void))

// check LWAB
static inline int lw_abi_attach(PVOID base) {
    PVOID *t = (PVOID *)base;
    if ((DWORD)t[LW_SLOT_MAGIC] != LW_ABI_MAGIC)
        return -1;
    lw_abi_base = t;
    return ((int (*)(void))t[LW_SLOT_ENTRY])();
}

#endif
