#include <stdint.h>
#include <interrupt.h>
#include <printk.h>
#include <tools.h>

#define SYS_WRITE 1

void dump_syscall_stack(syscall_regs_t *regs) {
    // 强制将结构体指针转换为 64 位整数数组指针
    uint64_t *raw_stack = (uint64_t *)regs;
    
    printk("\n=== SYSCALL STACK RAW DUMP (RSP: 0x%p) ===\n", regs);
    printk("Idx | Offset | Hex Value          | Possible Meaning\n");
    printk("----------------------------------------------------\n");

    // 一共压入了 20 个 64位值：15个手动 push + 5个硬件自动 push
    for (int i = 0; i < 20; i++) {
        uint64_t val = raw_stack[i];
        
        printk("[%02d] | +0x%02x | 0x%016lx | ", i, i * 8, val);

        // 简单的启发式推测，帮你快速定位你的参数跑到了哪里
        if (val == 1) {
            printk("<-- Might be RAX (SYS_WRITE)");
        } else if (val == 23) {
            printk("<-- Might be RDX (Length: 23)");
        } else if (val == 0) {
            printk("<-- Might be RDI (FD = 0) or 0-reg");
        } else if (val >= 0x400000 && val <= 0x402000) {
            // 用户态代码段通常在 0x400000 附近
            printk("<-- Might be RSI (Buffer Ptr) or RIP");
        } else if (val == 0x08 || val == 0x10 || val == 0x18 || val == 0x20 || val == 0x23 || val == 0x2b) {
            printk("<-- Might be CS / SS");
        } else if (val == 0x202 || val == 0x246) {
            // 常见的中断开启时的 RFLAGS 值
            printk("<-- Might be RFLAGS");
        }
        printk("\n");
    }
    printk("====================================================\n\n");
}
void syscall_handler(syscall_regs_t *regs) {
    //dump_syscall_stack(regs);
    uint64_t syscall_no = regs->rax;
    uint64_t ret = 0;
    printk("[SYSCALL] regs_ptr: %p, RAX=%d, RIP=%p\n", regs, (int)regs->rax, regs->rip);

    // dump_chunk((void*)0x8ffc0, 1);
    printk("arg1(fd)=%d, arg2(buf)=%p, arg3(len)=%d\n", (int)regs->rdi, regs->rsi, (int)regs->rdx);

    switch (syscall_no) {
        case SYS_WRITE:
            // 参数解析: rdi = fd, rsi = buffer, rdx = length
            int fd = (int)regs->rdi;
            char *buf = (char *)regs->rsi;
            size_t len = (size_t)regs->rdx;
            
            // 假设你内核里有一个处理打印的函数
            // ret = console_write(fd, buf, len);
            printk("[User] 0x%p: %s\n", buf, buf); // 测试用
            ret = len; 
            break;
            
        // case SYS_READ: ...
        
        default:
            printk("[lwos] Unhandled syscall: %d\n", syscall_no);
            ret = -1;
            break;
    }

    // 关键一步：将返回值写回栈上的 rax 位置。
    // 这样汇编 pop rax 时，用户态就能在 rax 里拿到返回值了。
    regs->rax = ret; 
}
extern video_t video;
extern void test_serial();