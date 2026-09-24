extern void monitor_main(void);
extern char __bss_start[], __bss_end[], __stack_top[];

#define LW_ABI_MAGIC 0x4241574cu

typedef struct {
    unsigned int magic;
    void (*entry)(void);
    void *bss_start;
    void *bss_end;
    void *stack_top;
} abi_table_t;

__attribute__((section(".abi")))
const abi_table_t abi_table = {
    .magic      = LW_ABI_MAGIC,
    .entry      = monitor_main,
    .bss_start  = __bss_start,
    .bss_end    = __bss_end,
    .stack_top  = __stack_top,
};