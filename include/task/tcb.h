#ifndef _TCB_H
#define _TCB_H

#include <stdint.h>

// 线程执行函数的函数指针类型
typedef void (*thread_func_t)(void* arg);

// 线程状态
typedef enum {
    TASK_RUNNING,
    TASK_READY,
    TASK_BLOCKED,
    TASK_DEAD
} task_state_t;

// 64 位线程控制块
typedef struct task_struct {
    uint64_t* kernel_stack; // 必须放在结构体的第一个位置 (偏移量为 0)，方便汇编寻址

    task_state_t state;
    uint32_t pid;
    char name[16];
    
    uint32_t priority;            // 静态优先级
    uint32_t ticks;               // 每次调度分配的时间片滴答数
    uint32_t elapsed_ticks;       // 总计运行的滴答数 (统计 CPU 占用)
    
    // 虚拟内存 (留作未来用户态进程切换 CR3 使用)
    uint64_t* pml4_dir;           
    
    // 链表节点 (用于就绪队列、阻塞队列)
    struct task_struct* next;     
    struct task_struct* prev;     
    
    // 栈溢出保护魔数 (放在结构体尾部，如果栈向下生长到底部，会先破坏这个魔数)
    uint64_t magic;
} task_struct_t;

typedef struct {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;   // 我们将巧妙利用 r12 传递函数参数
    uint64_t rbx;   // 我们将巧妙利用 rbx 传递函数指针
    uint64_t rbp;
    uint64_t rip;   // switch_to 最后的 ret 指令会弹出的地址
} thread_stack_t;

extern task_struct_t* current_thread;

void kernel_thread_entry(thread_func_t function, void* func_arg);
task_struct_t* thread_create(char* name, int priority, thread_func_t function, void* func_arg);
void task_a(void* arg);
void task_b(void* arg);
void schedule();
extern task_struct_t* main_thread;
extern task_struct_t* thread_a;
extern task_struct_t* thread_b;
void init_multitasking();

#endif