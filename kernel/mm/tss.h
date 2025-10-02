#ifndef _TSS_H
#define _TSS_H

#include <stdint.h>

#define TASK_RUNNING			0x00
#define TASK_INTERRUPTIBLE		0x01
#define TASK_UNINTERRUPTIBLE	0x02
#define TASK_STOPPED			0x04
#define TASK_TRACED				0x08
#define EXIT_DEAD				0x10
#define EXIT_ZOMBIE				0x20
#define EXIT_TRACED				0x30
#define TASK_DEAD				0x40
#define TASK_WAKEKILL			0x80
#define TASK_WAKING				0x0100
#define TASK_PARKED				0x0200
#define TASK_NOLOAD				0x0400
#define TASK_NEW				0x0800
#define TASK_STATE_MAX			0x1000
#define TAKE_KILLABLE			0x82

#pragma pack(1)
typedef struct {
	uint32_t prev_tss;
	uint32_t esp0;
	uint32_t ss0;
	uint32_t esp1;
	uint32_t ss1;
	uint32_t esp2;
	uint32_t ss2;
	uint32_t cr3;
	uint32_t eip;
	uint32_t eflags;
	uint32_t eax;
	uint32_t ecx;
	uint32_t edx;
	uint32_t ebx;
	uint32_t esp;
	uint32_t ebp;
	uint32_t esi;
	uint32_t edi;
	uint32_t es;
	uint32_t cs;
	uint32_t ss;
	uint32_t ds;
	uint32_t fs;
	uint32_t gs;
	uint32_t ldt;
	uint16_t trap;
	uint16_t iomap_base;
} tss_t;
typedef struct {
	uint32_t pid;
	uint32_t esp;
	uint32_t eip;
	uint32_t kernel_stack;
	task_t* next;
} task_t;
#pragma pack()

tss_t cpu_tss;
task_t* current_task = NULL;
task_t task_a, task_b;
extern void context_switch(uint32_t* old_esp, uint32_t new_esp);
void schedule(void) {
	task_t* next_task;
	task_t* prev_task;

	next_task = current_task->next;
	prev_task = current_task;

	if (next_task == current_task || next_task = > state != TASK_RUNNING) {
		return;
	}

	current_task = next_task;

	cpu_tss.esp0 = next_task->kernel_stack;

	context_switch(&(prev_task->esp), next_task->esp);
}


#endif
