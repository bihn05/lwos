// kernel/mm/pcb.h

#ifndef _PCB_H_
#define _PCB_H_

#include <stdint.h>
#include <mm/km.h>
#include <mm/vmm.h>
#include <string.h>
#include <descript.h>

#define TASK_READY   0
#define TASK_RUNNING 1
#define TASK_BLOCKED 2
#define TASK_DEAD    3

typedef struct {
    uint32_t gs, fs, es, ds;                        // 手动压入的段寄存器
    uint32_t edi, esi, ebp, esp_dummy, ebx, edx, ecx, eax; // pusha 压入的
    uint32_t eip, cs, eflags, user_esp, user_ss;    // 由硬件或我们伪造给 iret 使用的
} intr_frame_t;

typedef struct pcb {
    uint32_t esp;               // 当前内核栈的栈顶指针 (必须放在结构体第一个位置，方便汇编调用)
    uint32_t pid;               // 进程号
    uint32_t state;             // 进程状态
    uint32_t priority;          // 优先级 (可用于实现时间片多长)
    uint32_t ticks;             // 剩余时间片
    
    uint32_t kernel_stack;      // 该进程专属内核栈的栈底地址
    uint32_t page_dir_phys;     // 该进程专属的页目录物理地址 (CR3)
    
    struct pcb* next;           // 链表指针
} pcb_t;

extern pcb_t* current_task;
extern void switch_to(pcb_t* prev, pcb_t* next);

pcb_t* current_task = NULL;
pcb_t* ready_queue = NULL; // 简单的就绪队列

extern tss_t tss;

pcb_t* create_user_process(uint32_t entry_point) {
    // 1. 分配 PCB
    pcb_t* task = (pcb_t*)kmalloc(sizeof(pcb_t));
    memset(task, 0, sizeof(pcb_t));
    task->pid = 100; // 假定分配一个 PID
    task->state = TASK_READY;
    task->ticks = 5; // 分配 5 个时间片

    // 2. 为该进程分配一页专属的【内核栈】
    task->kernel_stack = (uint32_t)kmalloc(4096) + 4096; // 栈底在高地址
    
    // 3. 为该进程分配一页专属的【用户栈】(在虚拟内存高处)
    uint32_t user_stack_vaddr = 0xBFFFF000;
    vmm_alloc_map_region(user_stack_vaddr, 4096, PAGE_PRESENT | PAGE_RW | PAGE_USER);
    uint32_t user_stack_top = user_stack_vaddr + 4096;

    // 4. 精心布置内核栈，伪造中断现场
    // 将栈顶指针下移一个 intr_frame_t 的大小
    task->esp = task->kernel_stack - sizeof(intr_frame_t);
    intr_frame_t* frame = (intr_frame_t*)task->esp;

    // 清零通用寄存器
    memset(frame, 0, sizeof(intr_frame_t));

    // 填充用户态的段寄存器 (RPL=3)
    frame->ds = USER_DS;
    frame->es = USER_DS;
    frame->fs = USER_DS;
    frame->gs = USER_DS;

    // 填充 iret 返回时所需的特权级切换信息
    frame->user_ss = USER_DS;
    frame->user_esp = user_stack_top; // 进程在用户态使用的栈
    
    // EFLAGS: 开启中断 (IF=1, 第9位) | 预留位 (第1位必须为1)
    frame->eflags = 0x00000202; 
    
    frame->cs = USER_CS;              // 目标特权级代码段
    frame->eip = entry_point;         // 目标程序的执行入口

    // 将新任务加入队列
    task->next = ready_queue;
    ready_queue = task;

    return task;
}

void schedule() {
    if (!current_task || !ready_queue) return;

    current_task->ticks--;
    if (current_task->ticks > 0) return; // 时间片没用完，继续跑

    // 简单的轮转调度 (Round-Robin)
    pcb_t* next_task = current_task->next;
    if (!next_task) next_task = ready_queue; // 循环回到头

    if (current_task != next_task) {
        current_task->state = TASK_READY;
        next_task->state = TASK_RUNNING;
        next_task->ticks = 5; // 重置时间片

        // 【极为关键的 TSS 更新】
        // 告诉 CPU：下次这个 next_task 被中断打断时，请把现场保存到它的专属内核栈里！
        tss.esp0 = next_task->kernel_stack;

        pcb_t* prev = current_task;
        current_task = next_task;

        // 切换上下文
        switch_to(prev, next_task);
    }
}

#endif