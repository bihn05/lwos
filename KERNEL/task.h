#ifndef _TASK_H_
#define _TASK_H_

#include <stdint.h>

#define PAGE_SIZE 0x1000

#define TASK_NR 64
#define TASK_NAME_LEN 16
#define TASK_FILE_NR 16

typedef void target_t();

typedef struct task_t {
	uint32_t* stack;
} task_t;

task_t* a = (task_t*)0x90000;
task_t* b = (task_t*)0x91000;
extern void task_switch(task_t* next);
task_t* running_task() {
	asm volatile(
		"movl %esp, %eax\n"
		"andl $0xfffff000, %eax\n"
		);
}
void schedule() {
	task_t* current = running_task();
	putchar('\n');
	iouthex32(current);
	putchar('\n');
	task_t* next = (current == a) ? b : a;
	task_switch(next);
}
uint32_t thread_a() {
	while (1) {
		putchar('A');
		schedule();
	}
}
uint32_t thread_b() {
	while (1) {
		putchar('B');
		schedule();
	}
}
typedef struct task_frame_t {
	uint32_t edi;
	uint32_t esi;
	uint32_t ebx;
	uint32_t ebp;
	void (*eip)(void);
} task_frame_t;

static void task_create(task_t* task, target_t target) {
	uint32_t stack = (uint32_t)task + PAGE_SIZE;

	stack -= sizeof(task_frame_t);
	task_frame_t* frame = (task_frame_t*)stack;
	frame->ebx = 0x11111111;
	frame->esi = 0x22222222;
	frame->edi = 0x33333333;
	frame->ebp = 0x44444444;
	frame->eip = (void*)target;

	task->stack = (uint32_t*)stack;
}
void task_init() {
	task_create(a, thread_a);
	task_create(b, thread_b);
	schedule();
}

#endif