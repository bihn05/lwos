// process control block

#ifndef _PCB_H_
#define _PCB_H_

#include <stdint.h>
#include <stdlib.h>
#include <kernel.h>
#include <mm/km.h>

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
} task_t;

task_t* current_task = 0;
// table start
task_t* ready_queue = 0;

extern void context_switch(task_t* next);

void init_multitasking() {
    task_t* kernel_task = (task_t*)kmalloc(sizeof(task_t));
    printk("##KMALLOC kernel_task = 0x%08X\n", (uint32_t)kernel_task);

    asm volatile("mov %%cr3, %%eax; mov %%eax, %0" : "=m"(kernel_task->cr3) :: "eax");
    asm volatile("mov %%esp, %0" : "=r"(kernel_task->esp)); // store

    kernel_task->pid = 0;
    kernel_task->next = kernel_task;

    current_task = kernel_task;
    ready_queue = kernel_task;
}
void create_kernel_thread(void (*entry_point)()) {
    task_t* new_task = (task_t*)kmalloc(sizeof(task_t));

    // alloc kstack 4kb
    void* stack_base = kmalloc(4096);
    new_task->kernel_stack = (uint32_t)stack_base;

    // calc stack top
    uint32_t* stack_ptr = (uint32_t*)(stack_base + 4096 - sizeof(context_t));

    context_t* ctx = (context_t*)stack_ptr;
    ctx->edi = 0xdeadbeef;
    ctx->esi = 0;
    ctx->ebp = 0;
    ctx->espf = (uint32_t)stack_base + 4096;
    ctx->ebx = 0;
    ctx->edx = 0;
    ctx->ecx = 0;
    ctx->eax = 0;
    ctx->eip = (uint32_t)entry_point;
    ctx->cs = 0x08;
    ctx->eflags = 0x0202;

    new_task->esp = (uint32_t)stack_ptr;
    new_task->cr3 = current_task->cr3;

    new_task->pid = 1;

    new_task->next = ready_queue;

    printk("new task = 0x%08X\n", (uint32_t)new_task);
    printk("     esp = 0x%08X\n", new_task->esp);
    printk("     cr3 = 0x%08X\n", new_task->cr3);
    printk("     pid = %d\n", new_task->pid);
    printk("    next = 0x%08X\n", (uint32_t)new_task->next);
    printk("  krnstk = 0x%08X\n", new_task->kernel_stack);

    task_t* temp = ready_queue;
    while (temp->next != ready_queue)temp = temp->next;
    temp->next = new_task;
    // better use double directions chain table
}
void schedule() {

    if (!ready_queue)return;

	__asm("cli");
    task_t* next = current_task->next;

    if (next == current_task) {
        __asm("sti");
        return;
    }

    context_switch(next);
}
void task_a() {
    while (1) {
        printk("A");
        delay(100000);
    }
}
void task_b() {
    while (1) {
        printk("B");
        delay(100000);
    }
}

#endif