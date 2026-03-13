#include <task/tcb.h>
#include <mm.h>
#include <string.h>

#include <driver/kbc.h>

// 函数声明
extern intr_frame_t* switch_to(intr_frame_t* current_frame);
extern void kernel_thread_stub(void);
static uint32_t next_pid = 1;

// 线程统一的 C 语言外壳
void kernel_thread_bootstrap(thread_func_t function, void* arg) {
    __asm__ volatile("sti");
    function(arg);

    /* 未来这里换成 thread_exit() */
    while (1) {
        __asm__ volatile("hlt");
    }
}

task_struct_t* current_thread = NULL;
task_struct_t* main_thread = NULL;

static task_struct_t* ready_list = NULL;

intr_frame_t* schedule(intr_frame_t* current_frame) {
    if (current_thread == NULL || ready_list == NULL) {
        return current_frame;
    }

    task_struct_t* prev = current_thread;

    /* 保存当前线程被中断时的现场栈顶 */
    prev->kernel_stack = (uint64_t*)current_frame;

    /* 只有一个线程，继续自己 */
    if (current_thread->next == current_thread) {
        prev->state = TASK_RUNNING;
        return current_frame;
    }

    task_struct_t* next = current_thread->next;

    while (next != current_thread) {
        if (next->state == TASK_READY || next->state == TASK_RUNNING) {
            break;
        }
        next = next->next;
    }

    if (next == current_thread) {
        prev->state = TASK_RUNNING;
        return current_frame;
    }

    if (prev->state == TASK_RUNNING) {
        prev->state = TASK_READY;
    }

    next->state = TASK_RUNNING;
    current_thread = next;

    return (intr_frame_t*)next->kernel_stack;
}

static void ready_list_push_back(task_struct_t* t) {
    if (!t) return;

    if (ready_list == NULL) {
        ready_list = t;
        t->next = t;
        t->prev = t;
        return;
    }

    task_struct_t* tail = ready_list->prev;

    tail->next = t;
    t->prev = tail;
    t->next = ready_list;
    ready_list->prev = t;
}

static void ready_list_remove(task_struct_t* t) {
    if (!t || !ready_list) return;

    if (t->next == t) {
        ready_list = NULL;
        t->next = NULL;
        t->prev = NULL;
        return;
    }

    if (ready_list == t) {
        ready_list = t->next;
    }

    t->prev->next = t->next;
    t->next->prev = t->prev;

    t->next = NULL;
    t->prev = NULL;
}
static inline uint64_t read_rsp(void) {
    uint64_t rsp;
    __asm__ volatile("mov %%rsp, %0" : "=r"(rsp));
    return rsp;
}
void init_multitasking() {
    main_thread = (task_struct_t*)kmalloc(4096);
    if (!main_thread) {
        printk("init_multitasking: kmalloc main_thread failed\n");
        while (1) __asm__ volatile("hlt");
    }

    memset(main_thread, 0, 4096);

    main_thread->pid = 0;
    strncpy(main_thread->name, "main", sizeof(main_thread->name) - 1);
    main_thread->priority = 1;
    main_thread->ticks = 1;
    main_thread->elapsed_ticks = 0;
    main_thread->magic = 0x19980812;
    main_thread->pml4_dir = get_cr3();   // 不要写死 0x200000
    main_thread->state = TASK_RUNNING;

    /* 关键：主线程没有伪造栈，必须记录当前真实 RSP */
    main_thread->kernel_stack = (uint64_t*)read_rsp();

    ready_list = NULL;
    ready_list_push_back(main_thread);

    current_thread = main_thread;
}

task_struct_t* thread_create(
    uint64_t pml4_pa,
    char* name,
    int priority,
    thread_func_t function,
    void* func_arg
) {
    task_struct_t* thread = (task_struct_t*)kmalloc(4096);
    if (!thread) return NULL;

    memset(thread, 0, 4096);

    thread->pid = next_pid++;
    strncpy(thread->name, name, 15);
    thread->state = TASK_READY;
    thread->priority = priority;
    thread->ticks = priority;
    thread->elapsed_ticks = 0;
    thread->magic = 0x19980812;
    thread->pml4_dir = pml4_pa;
    thread->next = NULL;
    thread->prev = NULL;

    uint64_t stack_top = (uint64_t)thread + 4096;
    stack_top -= sizeof(intr_frame_t);
    intr_frame_t* frame = (intr_frame_t*)stack_top;
    memset(frame, 0, sizeof(*frame));

    /*
     * 关键点：
     * 中断恢复后会 pop 通用寄存器，再 iretq 到 rip。
     * 所以我们把“首次运行”伪装成：从中断返回到 kernel_thread_stub。
     */
    frame->rdi = (uint64_t)function;     // 第一个参数
    frame->rsi = (uint64_t)func_arg;     // 第二个参数
    frame->rip = (uint64_t)kernel_thread_stub;
    frame->cs = 0x08;                    // 你的内核代码段选择子，按你的 GDT 改
    frame->rflags = 0x202;               // IF=1

    frame->rsp = (uint64_t)thread + 4096;
    frame->ss  = 0x10;

    thread->kernel_stack = (uint64_t*)frame;
    return thread;
}

task_struct_t* thread_start(
    uint64_t pml4_pa,
    char* name,
    int priority,
    thread_func_t function,
    void* func_arg
) {
    task_struct_t* t = thread_create(pml4_pa, name, priority, function, func_arg);
    if (!t) {
        printk("Failed create thread\n");
        return NULL;
    }

    ready_list_push_back(t);
    return t;
}

void timer_tick_accounting(intr_frame_t* frame) {
    outb(0x20, 0x20);   // 主 PIC EOI
}