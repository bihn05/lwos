/*
 * MONITOR.BIN 的映像头: 只填 slot 0x00-0x04
 * loader 的 P 命令靠它校验、清 bss、换栈、跳到入口
 */

#include "abi.h"
#include "stdint.h"

extern void monitor_main(void);
extern char __bss_start[], __bss_end[], __stack_top[];

const PVOID lw_head[LW_SLOT_STACK_TOP + 1] __attribute__((section(".abi"), used, aligned(16))) = {
    [LW_SLOT_MAGIC]     = (PVOID)LW_ABI_MAGIC,
    [LW_SLOT_ENTRY]     = (PVOID)monitor_main,
    [LW_SLOT_BSS_START] = (PVOID)__bss_start,
    [LW_SLOT_BSS_END]   = (PVOID)__bss_end,
    [LW_SLOT_STACK_TOP] = (PVOID)__stack_top
};
