#include "test.h"
#include "sys.h"

int main() {
    //syscall3(2, 0x123, 0x456, 0x892);
    write(1, "hello from user space!\n", 23);
    write(1, "2ello from user space!\n", 23);
    write(1, "3ello from user space!\n", 23);
    write(1, "4eelo from user space!\n", 23);
    while (1);
    return HELLO;
}