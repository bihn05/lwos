#include "test.h"
#include "sys.h"

int main() {
    //syscall3(2, 0x123, 0x456, 0x892);
    while (1) {
        write(1, "U\0", 2);
        for (int i = 0; i < 10000000; i++) {
            ;
        }
    }
    return HELLO;
}