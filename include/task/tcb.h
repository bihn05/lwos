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

typedef struct intr_frame {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t rbp;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} intr_frame_t;

// 64 位线程控制块
typedef struct task_struct {
    uint64_t* kernel_stack;
    intr_frame_t saved_frame; // 规范化寄存器帧

    task_state_t state;
    uint32_t pid;
    char name[16];
    
    uint32_t priority;            // 静态优先级
    uint32_t ticks;               // 每次调度分配的时间片滴答数
    uint32_t elapsed_ticks;       // 总计运行的滴答数 (统计 CPU 占用)
    uint64_t wakeup_tick;         // 醒了
    
    uint64_t pml4_dir;           
    struct task_struct* next;     
    struct task_struct* prev;
    struct task_struct* reap_next;  // 死了
    struct task_struct* sleep_next; // 睡了
    struct task_struct* wait_next;
    
    __attribute__((aligned(16))) uint8_t fxstate[512];
    
    // 栈溢出保护魔数
    uint64_t magic;
} task_struct_t;

typedef struct wait_queue {
    task_struct_t* head;
    task_struct_t* tail;
} wait_queue_t;

extern wait_queue_t kbd_wait_queue;

typedef struct {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;   // 我们将巧妙利用 r12 传递函数参数
    uint64_t rbx;   // 我们将巧妙利用 rbx 传递函数指针
    uint64_t rbp;
    uint64_t rip;   // switch_to 最后的 ret 指令会弹出的地址
} thread_stack_t;

typedef struct thread_context {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t rbx;
    uint64_t rbp;
    uint64_t rip;
} thread_context_t;

extern task_struct_t* current_thread;
extern task_struct_t* main_thread;

void kernel_thread_bootstrap(thread_func_t function, void* arg);
intr_frame_t* schedule(intr_frame_t* current_frame);
void init_multitasking();
task_struct_t* thread_create(
    uint64_t pml4_pa,
    char* name,
    int priority,
    thread_func_t function,
    void* func_arg
);
task_struct_t* thread_start(
    uint64_t pml4_pa,
    char* name,
    int priority,
    thread_func_t function,
    void* func_arg
);
task_struct_t* process_create_from_elf(
    uint64_t user_pm4,
    const char* name,
    uint64_t entry_point,
    uint64_t user_stack_top
);
void timer_tick_accounting(intr_frame_t* frame);
void fpu_state_init(task_struct_t* t);
void reap_dead_threads(void);
void thread_sleep_ticks(uint64_t ticks);
//void ready_list_push_back(task_struct_t* t);
void ready_list_remove(task_struct_t* t);
task_struct_t* pick_next_runnable(void);
void thread_sleep_ms(uint64_t ms);
void wait_queue_init(wait_queue_t* q);
void thread_unblock(task_struct_t* t);
void thread_block(task_state_t blocked_state);
task_struct_t* wait_queue_pop(wait_queue_t* q);
void wait_queue_push(wait_queue_t* q, task_struct_t* t);
void thread_block_on(wait_queue_t* q);
void wake_up_one(wait_queue_t* q);
void wake_up_all(wait_queue_t* q);

extern void context_switch(task_struct_t* prev, task_struct_t* next); // 弃用
extern void save_current_frame_and_switch(intr_frame_t* prev_frame,
                                          intr_frame_t* next_frame);

#endif