
__attribute__((section(".text.start")))
void monitor_main() {
    *(char*)0xb8000 = 'a'; // test abi
    while (1);
}