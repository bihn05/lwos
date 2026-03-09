#include "test.h"
#include "sys.h"

int main() {
    //syscall3(2, 0x123, 0x456, 0x892);
    int a = write(1, "hello from user space!\n", 23);
    if (a == 23) {
        write(1, "write syscall works correctly!\n", 31);
    }
    while (1);
    return HELLO;
}