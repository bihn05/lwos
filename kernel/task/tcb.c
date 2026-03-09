#include <task/tcb.h>
#include <mm.h>
#include <string.h>
// 函数声明
extern void switch_to(task_struct_t* prev, task_struct_t* next);
extern void kernel_thread_stub(void);
static uint32_t next_pid = 1;

// 线程统一的 C 语言外壳
void kernel_thread_entry(thread_func_t function, void* func_arg) {
    // 必须开启中断！因为如果它是被调度器切换过来的，中断标志 IF 可能是关闭的
    __asm__ volatile("sti"); 
    
    // 执行真正的线程任务
    function(func_arg);
    
    // TODO: 这里未来应该调用 thread_exit() 将自己移出调度队列并回收内存
    while(1) { __asm__ volatile("hlt"); }
}
task_struct_t* thread_create(uint64_t pml4_pa, char* name, int priority, thread_func_t function, void* func_arg) {
    // 1. 分配一个 4KB 页作为 TCB 和栈的混合体
    task_struct_t* thread = (task_struct_t*)kmalloc(4096);
    if (!thread) return NULL;

    memset(thread, 0, 4096);

    // 2. 初始化 TCB 基础属性
    thread->pid = next_pid++;
    strncpy(thread->name, name, 15);
    thread->state = TASK_READY;
    thread->priority = priority;
    thread->ticks = priority; 
    thread->elapsed_ticks = 0;
    thread->magic = 0x19980812; // 设置栈溢出保护魔数
    thread->pml4_dir = pml4_pa; // 未来用户态进程切换 CR3 使用

    // 3. 计算并伪造内核栈
    // 栈底在这 4KB 页的最高地址处
    uint64_t stack_top = (uint64_t)thread + 4096;
    
    // 预留出一个 thread_stack_t 的空间
    stack_top -= sizeof(thread_stack_t);
    thread_stack_t* kstack = (thread_stack_t*)stack_top;

    // 4. 填充跳板所需的寄存器快照
    kstack->rbx = (uint64_t)function;  // 目标函数
    kstack->r12 = (uint64_t)func_arg;  // 函数参数
    kstack->rbp = 0;                   // 栈底设为0，方便未来回溯调用栈
    kstack->r13 = 0;
    kstack->r14 = 0;
    kstack->r15 = 0;
    
    // 5. 核心：将 RIP 指向汇编跳板
    kstack->rip = (uint64_t)kernel_thread_stub;

    // 6. 保存计算好的栈顶指针到 TCB 首部
    thread->kernel_stack = (uint64_t*)kstack;

    return thread;
}

void schedule() {
    // 防御性编程：如果没有任务，或者只有一个任务，不切换
    if (current_thread == NULL || current_thread->next == current_thread) {
        return; 
    }

    // 取出下一个任务
    task_struct_t* next = current_thread->next;
    task_struct_t* prev = current_thread;

    current_thread = next;

    switch_to(prev, next);
}

task_struct_t* current_thread = NULL;

void task_a(void* arg) {
    while (1) {
        printk("A");
        // 适当延时，防止打印太快看不清
        for(volatile int i=0; i<1000; i++); 
    }
}
void task_b(void* arg) {
    while (1) {
        printk("B");
        for(volatile int i=0; i<1000; i++); 
    }
}
task_struct_t* main_thread;

task_struct_t* thread_a = NULL;
task_struct_t* thread_b = NULL;
void init_multitasking() {
    // 为当前正在运行的 kernel_init 申请一个 TCB
    main_thread = (task_struct_t*)kmalloc(4096);
    main_thread->pid = 0;
    main_thread->pml4_dir = 0x200000;
    strcpy(main_thread->name, "main");
    main_thread->state = TASK_RUNNING;
    // 注意：不需要为它伪造栈，因为此时 RSP 已经是指向正确的内核栈了
    
    current_thread = main_thread;

    // 连成环：main -> A -> B -> main
    main_thread->next = thread_a;
    thread_a->next = thread_b;
    thread_b->next = main_thread;
}