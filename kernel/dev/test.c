// test.c

void sys_printk(const char* str) {
    __asm volatile (
        "int $0x80"
        :
        : "a"(1), "b"(str) // syscall number 1 for print, and the string in ebx
    );
}

int main() {
    sys_printk("Hello from the test process!\n");
    return 666;
}