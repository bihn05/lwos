// process control block

#ifndef _PCB_H_
#define _PCB_H_

#include <debug.h>
#include <stdint.h>
#include <stdlib.h>
#include <kernel.h>
#include <mm/km.h>

volatile uint32_t system_ticks = 0;

typedef enum {
    TASK_RUNNING,
    TASK_READY,
    TASK_BLOCKED,
} task_state_t;

typedef struct {
    uint32_t edi, esi, ebp, espf, ebx, edx, ecx, eax;
    uint32_t eip, cs, eflags;
} context_t;

typedef struct task {
    uint32_t esp;
    uint32_t cr3;
    uint32_t pid;
    struct task* next;
    uint32_t kernel_stack;

    task_state_t state;
    uint32_t sleep_ticks;
} task_t;

task_t* current_task = 0;
// table start
task_t* ready_queue = 0;

extern void context_switch(task_t* next);
extern void switch_task(task_t* prev, task_t* next);
extern void kernel_thread_entry();

void init_multitasking() {
    task_t* kernel_task = (task_t*)kmalloc(sizeof(task_t));
    printk("##KMALLOC kernel_task = 0x%08X\n", (uint32_t)kernel_task);

    asm volatile("mov %%cr3, %%eax; mov %%eax, %0" : "=m"(kernel_task->cr3) :: "eax");
    asm volatile("mov %%esp, %0" : "=r"(kernel_task->esp)); // store

    kernel_task->pid = 0;
    kernel_task->next = kernel_task;
    kernel_task->state = TASK_READY;

    current_task = kernel_task;
    ready_queue = kernel_task;
}
void task_starter(void (*entry_point)()) {
    __asm volatile("sti"); // enable interrupts
    entry_point();
    while (1);
}
void create_kernel_thread(void (*entry_point)()) {
    task_t* new_task = (task_t*)kmalloc(sizeof(task_t));

    // alloc kstack 4kb
    uint8_t* stack_base = (uint8_t*)kmalloc(4096);

    // calc stack top
    uint32_t* stack_ptr = (uint32_t*)(((uint32_t)stack_base + 4096) & ~0xF);

    *(--stack_ptr) = (uint32_t)kernel_thread_entry; // context switch entry point
    *(--stack_ptr) = 0; // pop ebp
    *(--stack_ptr) = (uint32_t)entry_point; // pop ebx
    *(--stack_ptr) = 0; // pop esi
    *(--stack_ptr) = 0; // pop edi

    new_task->esp = (uint32_t)stack_ptr;
    new_task->cr3 = current_task->cr3;
    new_task->kernel_stack = (uint32_t)stack_base;
    new_task->state = TASK_READY;

    static uint32_t next_pid = 1;
    new_task->pid = next_pid++;

    new_task->next = ready_queue;

    printk("     pid = %d\n", new_task->pid);
    printk("new task = 0x%08X\n", (uint32_t)new_task);
    printk("     esp = 0x%08X\n", new_task->esp);
    printk("    next = 0x%08X\n", (uint32_t)new_task->next);
    printk("  krnstk = 0x%08X\n", new_task->kernel_stack);
    printk("     cr3 = 0x%08X\n", new_task->cr3);

    task_t* temp = ready_queue;
    while (temp->next != ready_queue)temp = temp->next;
    temp->next = new_task;
    // better use double directions chain table
}
void schedule() {
    if (!ready_queue || !current_task) return;

    task_t* prev = current_task;
    task_t* next = current_task->next;

    while (next->state != TASK_READY && next != prev) {
        next = next->next;
    }

    if (prev != next) {
        current_task = next;
        switch_task(prev, next);
    }
}

void sleep(uint32_t ticks) {
    if (ticks == 0) return;

    current_task->sleep_ticks = system_ticks + ticks;
    current_task->state = TASK_BLOCKED;
    schedule();
}
void do_timer_tick() {
    system_ticks++;

    if (!ready_queue) return;

    // wake up tasks
    task_t* temp = ready_queue;
    do {
        if (temp->state == TASK_BLOCKED && temp->sleep_ticks <= system_ticks) {
            temp->state = TASK_READY;
        }
        temp = temp->next;
    } while (temp != ready_queue);

    schedule();
}
void task_a() {
    while (1) {
        printk("PI=%f\n", 3.14159265358979323846);
        sleep(314);
    }
}
void task_b() {
    while (1) {
        printk(" E=%f\n", 2.71828182845904523536);
        sleep(272);
    }
}

#endif