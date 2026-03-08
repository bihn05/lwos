#include <stdint.h>

#define SYS_WRITE 1

uint64_t syscall3(uint64_t sys_num, uint64_t arg1, uint64_t arg2, uint64_t arg3) {
    uint64_t ret;
    asm volatile (
        "int $0x80"
        : "=a" (ret)                  // 期望返回值在 rax 中
        : "a" (sys_num),              // 系统调用号放入 rax
          "D" (arg1),                 // arg1 放入 rdi
          "S" (arg2),                 // arg2 放入 rsi
          "d" (arg3)                  // arg3 放入 rdx
        : "rcx", "r11", "memory"      // 破坏列表，告诉编译器内存可能被内核改变
    );
    return ret;
}

int write(int fd, const char *buf, int len) {
    return syscall3(SYS_WRITE, (uint64_t)fd, (uint64_t)buf, (uint64_t)len);
}