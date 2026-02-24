// kernel/int/syscall.c

#include <stdint.h>
#include <kernel.h>
#include <mm/pcb.h>

void syscall_handler(intr_frame_t* frame) {
    uint32_t syscall_num = frame->eax;

    switch (syscall_num) {
        case 1: // system print kernel
        printk((char*)frame->ebx);
        break;

        case 2: // system exit
        printk("Process %d exited with code %d\n", current_task->pid, frame->ebx);
        current_task->state = TASK_DEAD;
        // schedule(); // 切换到下一个任务
        break;

        default:
        printk("Unknown syscall: %d\n", syscall_num);
        break;
    }
}