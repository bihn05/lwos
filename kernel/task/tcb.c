#include <task/tcb.h>
#include <mm.h>
#include <string.h>

//#include <driver/kbc.h>
#include <pic.h>

// 函数声明
extern intr_frame_t* switch_to(intr_frame_t* current_frame);
extern void kernel_thread_stub(void);
static uint32_t next_pid = 1;
static volatile uint64_t timer_ticks = 0;

// 线程统一的 C 语言外壳
void kernel_thread_bootstrap(thread_func_t function, void* arg) {
    __asm__ volatile("sti");
    function(arg);
}
static inline uint64_t read_cr3(void) {
    uint64_t v;
    __asm__ volatile("mov %%cr3, %0" : "=r"(v));
    return v;
}

static inline void write_cr3(uint64_t v) {
    __asm__ volatile("mov %0, %%cr3" :: "r"(v) : "memory");
}

static inline void fxsave_state(void* p) {
    __asm__ volatile("fxsave %0" : "=m"(*(uint8_t (*)[512])p));
}

static inline void fxrstor_state(void* p) {
    __asm__ volatile("fxrstor %0" :: "m"(*(uint8_t (*)[512])p));
}

task_struct_t* current_thread = NULL;
task_struct_t* main_thread = NULL;

static task_struct_t* ready_list = NULL;

intr_frame_t* schedule(intr_frame_t* current_frame) {
    if (current_thread == NULL || ready_list == NULL) {
        return current_frame;
    }

    task_struct_t* prev = current_thread;

    uint64_t cs = *(uint64_t*)((uint8_t*)current_frame + offsetof(intr_frame_t, cs));
    if ((cs & 3) == 0) {
        /* ring0 -> ring0 中断：硬件没压 rsp/ss，需要手工补齐 */

        /* 先逐字段复制已有部分 */
        prev->saved_frame.r15    = current_frame->r15;
        prev->saved_frame.r14    = current_frame->r14;
        prev->saved_frame.r13    = current_frame->r13;
        prev->saved_frame.r12    = current_frame->r12;
        prev->saved_frame.r11    = current_frame->r11;
        prev->saved_frame.r10    = current_frame->r10;
        prev->saved_frame.r9     = current_frame->r9;
        prev->saved_frame.r8     = current_frame->r8;
        prev->saved_frame.rsi    = current_frame->rsi;
        prev->saved_frame.rdi    = current_frame->rdi;
        prev->saved_frame.rbp    = current_frame->rbp;
        prev->saved_frame.rdx    = current_frame->rdx;
        prev->saved_frame.rcx    = current_frame->rcx;
        prev->saved_frame.rbx    = current_frame->rbx;
        prev->saved_frame.rax    = current_frame->rax;
        prev->saved_frame.rip    = current_frame->rip;
        prev->saved_frame.cs     = current_frame->cs;
        prev->saved_frame.rflags = current_frame->rflags;

        /*
         * 对 ring0 同级中断，被打断前的 rsp 就是：
         *   当前短帧起始地址 + 短帧实际大小
         *
         * 这里不能用 sizeof(intr_frame_t)，因为完整 frame 比实际硬件压入的大。
         * 实际 ring0 短帧大小 = 到 rflags 为止。
         */
        prev->saved_frame.rsp =
            (uint64_t)((uint8_t*)current_frame + offsetof(intr_frame_t, rsp));

        /* 内核数据段 */
        prev->saved_frame.ss = 0x10;

        prev->kernel_stack = (uint64_t*)&prev->saved_frame;
    } else {
        /*
         * ring3 -> ring0 中断：硬件已经压了完整的 rsp/ss
         * 这时 current_frame 真的是完整 frame，可以整块复制
         */
        prev->saved_frame = *current_frame;
        prev->kernel_stack = (uint64_t*)&prev->saved_frame;
    }

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

    // 保存旧任务的 FPU/SSE 状态 
    fxsave_state(prev->fxstate);

    // 如有需要，再切地址空间
    if (prev->pml4_dir != next->pml4_dir && next->pml4_dir != 0) {
        write_cr3(next->pml4_dir);
    }

    // 恢复新任务的 FPU/SSE 状态
    fxrstor_state(next->fxstate);

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

void ready_list_remove(task_struct_t* t) {
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

    fpu_state_init(main_thread);

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

    fpu_state_init(thread);

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

    thread->saved_frame = *frame;
    thread->kernel_stack = (uint64_t*)&thread->saved_frame;
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
task_struct_t* process_create_from_elf(
    uint64_t user_pm4,
    const char* name,
    uint64_t entry_point,
    uint64_t user_stack_top
) {
    task_struct_t* t = (task_struct_t*)kmalloc(4096);
    if (!t) return NULL;

    memset(t, 0, 4096);

    t->pid = next_pid++;
    strncpy(t->name, name, 15);
    t->state = TASK_READY;
    t->priority = 1;
    t->ticks = 1;
    t->elapsed_ticks = 0;
    t->magic = 0x19980812;
    t->pml4_dir = user_pm4;

    uint64_t stack_top = (uint64_t)t + 4096;
    stack_top -= sizeof(intr_frame_t);

    intr_frame_t* frame = (intr_frame_t*)stack_top;
    memset(frame, 0, sizeof(*frame));

    frame->rip    = entry_point;
    frame->cs     = 0x1b;
    frame->rflags = 0x202;
    frame->rsp    = user_stack_top;
    frame->ss     = 0x23;

    t->saved_frame = *frame;
    t->kernel_stack = (uint64_t*)&t->saved_frame;
    //t->kernel_stack = (uint64_t*)frame;
    ready_list_push_back(t);
    return t;
}
void fpu_state_init(task_struct_t* t) {
    memset(t->fxstate, 0, 512);
    __asm__ volatile("fninit");
    __asm__ volatile("fxsave %0" : "=m"(*(uint8_t (*)[512])t->fxstate));
}

static task_struct_t* zombie_list = NULL;
static task_struct_t* reap_list = NULL;

static void zombie_list_push(task_struct_t* t) {
    t->next = zombie_list;
    zombie_list = t;
}

static void reap_list_push(task_struct_t* t) {
    if (!t) return;
    t->reap_next = reap_list;
    reap_list = t;
}

extern void thread_exit_switch(intr_frame_t* next_frame);

void thread_exit(void) {
    __asm__ volatile("cli");

    task_struct_t* cur = current_thread;
    if (!cur) {
        while (1) __asm__ volatile("hlt");
    }

    cur->state = TASK_DEAD;
    ready_list_remove(cur);
    reap_list_push(cur);

    if (ready_list == NULL) {
        printk("thread_exit: no runnable threads left\n");
        while (1) __asm__ volatile("hlt");
    }

    task_struct_t* next = ready_list;
    while (1) {
        if (next->state == TASK_READY || next->state == TASK_RUNNING) {
            break;
        }
        next = next->next;
        if (next == ready_list) {
            printk("thread_exit: no runnable next\n");
            while (1) __asm__ volatile("hlt");
        }
    }

    if (cur->pml4_dir != next->pml4_dir && next->pml4_dir != 0) {
        write_cr3(next->pml4_dir);
    }

    next->state = TASK_RUNNING;
    current_thread = next;

    thread_exit_switch((intr_frame_t*)next->kernel_stack);

    while (1) __asm__ volatile("hlt");
}

void reap_dead_threads(void) {
    __asm__ volatile("cli");

    task_struct_t* list = reap_list;
    reap_list = NULL;

    __asm__ volatile("sti");

    while (list) {
        task_struct_t* next = list->reap_next;

        printk("reap pid=%d name=%s\n", list->pid, list->name);

        // 如果以后用户进程有独立页表，这里顺便 free 页表
        // if (list->pml4_dir != kernel_pml4) destroy_address_space(list->pml4_dir);

        kfree(list);

        list = next;
    }
}

static task_struct_t* sleep_list = NULL;

static void sleep_list_push(task_struct_t* t) {
    t->sleep_next = sleep_list;
    sleep_list = t;
}
task_struct_t* pick_next_runnable(void) {
    if (!ready_list) return NULL;

    task_struct_t* t = ready_list;
    do {
        if (t->state == TASK_READY || t->state == TASK_RUNNING) {
            return t;
        }
        t = t->next;
    } while (t != ready_list);

    return NULL;
}

void thread_sleep_ticks(uint64_t ticks) {
    if (ticks == 0) {
        return;
    }

    __asm__ volatile("cli");

    task_struct_t* cur = current_thread;
    if (!cur) {
        __asm__ volatile("sti");
        return;
    }

    cur->wakeup_tick = timer_ticks + ticks;
    cur->state = TASK_BLOCKED;

    ready_list_remove(cur);
    sleep_list_push(cur);

    task_struct_t* next = pick_next_runnable();
    if (!next) {
        printk("thread_sleep_ticks: no runnable thread\n");
        while (1) {
            __asm__ volatile("hlt");
        }
    }

    if (cur->pml4_dir != next->pml4_dir && next->pml4_dir != 0) {
        write_cr3(next->pml4_dir);
    }

    next->state = TASK_RUNNING;
    current_thread = next;

    save_current_frame_and_switch(&cur->saved_frame,
                                  (intr_frame_t*)next->kernel_stack);

    /* 将来被 timer 唤醒并调度回来后，从这里继续 */
    __asm__ volatile("sti");
}

void timer_tick_accounting(intr_frame_t* frame) {
    (void)frame;

    timer_ticks++;

    if (current_thread) {
        current_thread->elapsed_ticks++;
    }

    task_struct_t** pp = &sleep_list;
    while (*pp) {
        task_struct_t* t = *pp;

        if (t->wakeup_tick <= timer_ticks) {
            *pp = t->sleep_next;
            t->sleep_next = NULL;

            t->state = TASK_READY;
            ready_list_push_back(t);
        } else {
            pp = &t->sleep_next;
        }
    }

    outb(0x20, 0x20);   // 主 PIC EOI
}

void thread_sleep_ms(uint64_t ms) {
    uint64_t ticks = (ms * FREQ_T0 + 999) / 1000;  // 向上取整
    if (ticks == 0) ticks = 1;
    thread_sleep_ticks(ticks);
}

void thread_block(task_state_t blocked_state) {
    if (blocked_state != TASK_BLOCKED) {
        printk("thread_block: invalid state\n");
        return;
    }

    __asm__ volatile("cli");

    task_struct_t* cur = current_thread;
    if (!cur) {
        __asm__ volatile("sti");
        return;
    }

    cur->state = blocked_state;
    ready_list_remove(cur);

    task_struct_t* next = pick_next_runnable();
    if (!next) {
        printk("thread_block: no runnable thread\n");
        while (1) {
            __asm__ volatile("hlt");
        }
    }

    if (cur->pml4_dir != next->pml4_dir && next->pml4_dir != 0) {
        write_cr3(next->pml4_dir);
    }

    next->state = TASK_RUNNING;
    current_thread = next;

    thread_exit_switch((intr_frame_t*)next->kernel_stack);

    while (1) {
        __asm__ volatile("hlt");
    }
}

void thread_unblock(task_struct_t* t) {
    if (!t) return;

    __asm__ volatile("cli");

    if (t->state != TASK_BLOCKED) {
        __asm__ volatile("sti");
        return;
    }

    t->state = TASK_READY;
    ready_list_push_back(t);

    __asm__ volatile("sti");
}

wait_queue_t kbd_wait_queue;

void wait_queue_init(wait_queue_t* q) {
    q->head = NULL;
    q->tail = NULL;
}

void wait_queue_push(wait_queue_t* q, task_struct_t* t) {
    t->wait_next = NULL;

    if (q->tail == NULL) {
        q->head = t;
        q->tail = t;
        return;
    }

    q->tail->wait_next = t;
    q->tail = t;
}

task_struct_t* wait_queue_pop(wait_queue_t* q) {
    task_struct_t* t = q->head;
    if (!t) return NULL;

    q->head = t->wait_next;
    if (q->head == NULL) {
        q->tail = NULL;
    }

    t->wait_next = NULL;
    return t;
}

void thread_block_on(wait_queue_t* q) {
    task_struct_t* cur = current_thread;
    if (!cur) {
        return;
    }

    cur->state = TASK_BLOCKED;
    ready_list_remove(cur);
    wait_queue_push(q, cur);

    task_struct_t* next = pick_next_runnable();
    if (!next) {
        printk("thread_block_on: no runnable thread\n");
        while (1) {
            __asm__ volatile("hlt");
        }
    }

    if (cur->pml4_dir != next->pml4_dir && next->pml4_dir != 0) {
        write_cr3(next->pml4_dir);
    }

    next->state = TASK_RUNNING;
    current_thread = next;

    save_current_frame_and_switch(&cur->saved_frame,
                                  (intr_frame_t*)next->kernel_stack);

    /* 线程将来被唤醒后，会从这里继续 */
    __asm__ volatile("sti");
}

void wake_up_one(wait_queue_t* q) {
    __asm__ volatile("cli");

    task_struct_t* t = wait_queue_pop(q);
    if (t && t->state == TASK_BLOCKED) {
        t->state = TASK_READY;
        ready_list_push_back(t);
    }

    __asm__ volatile("sti");
}

void wake_up_all(wait_queue_t* q) {
    __asm__ volatile("cli");

    task_struct_t* t;
    while ((t = wait_queue_pop(q)) != NULL) {
        if (t->state == TASK_BLOCKED) {
            t->state = TASK_READY;
            ready_list_push_back(t);
        }
    }

    __asm__ volatile("sti");
}

