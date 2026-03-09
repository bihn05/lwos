#ifndef _PROC_MM_H
#define _PROC_MM_H

#include <stdint.h>
#include <mm/pmm.h>
#include <mm/pt.h>
#include <mm/vmm.h>

#define PAGE_SIZE 0x1000ULL
#define PAGE_MASK 0xFFFFFFFFFFFFF000ULL

#define PTE_P   (1ULL << 0)
#define PTE_RW  (1ULL << 1)
#define PTE_US  (1ULL << 2)
#define PTE_PWT (1ULL << 3)
#define PTE_PCD (1ULL << 4)
#define PTE_A   (1ULL << 5)
#define PTE_D   (1ULL << 6)
#define PTE_PS  (1ULL << 7)
#define PTE_G   (1ULL << 8)
#define PTE_NX  (1ULL << 63)

#define USER_TEXT_BASE      0x0000000000400000ULL
#define USER_STACK_TOP      0x0000000070000000ULL
#define USER_STACK_SIZE     0x0000000000400000ULL   // 4 MiB

#define KERNEL_STACK_REGION_BASE 0xFFFFFFFF81000000ULL
#define KERNEL_STACK_SIZE        0x4000ULL           // 16 KiB
#define KERNEL_STACK_STRIDE      0x5000ULL           // 多一页 guard

#define PAGE_FAULT_STACK_BASE    0xFFFFFFFFC1000000ULL
#define PAGE_FAULT_STACK_SIZE    0x4000ULL           // 16 KiB
#define PAGE_FAULT_STACK_TOP     (PAGE_FAULT_STACK_BASE + PAGE_FAULT_STACK_SIZE)

#define DOUBLE_FAULT_STACK_BASE  0xFFFFFFFFC2000000ULL
#define DOUBLE_FAULT_STACK_SIZE  0x4000ULL           // 16 KiB
#define DOUBLE_FAULT_STACK_TOP   (DOUBLE_FAULT_STACK_BASE + DOUBLE_FAULT_STACK_SIZE)

#define LEGACY_FB_PHYS_BASE   0x000A0000ULL
#define LEGACY_FB_SIZE        0x00009600ULL
#define LEGACY_FB_VIRT_BASE   0xFFFFFFFFC3000000ULL
typedef struct trap_frame {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t vector;
    uint64_t error_code;
    uint64_t rip, cs, rflags, rsp, ss;
} trap_frame_t;

typedef struct vm_space {
    uint64_t pml4_pa;
    uint64_t brk_start;
    uint64_t brk_end;
} vm_space_t;

typedef struct thread {
    uint64_t kernel_rsp;      // 调度时保存的内核 rsp
    uint64_t user_rsp;        // 首次进入用户态时用
    uint64_t user_rip;
    uint64_t kstack_base;
    uint64_t kstack_top;
    trap_frame_t* initial_tf; // 放在内核栈顶附近
} thread_t;

typedef struct process {
    vm_space_t vm;
    thread_t main_thread;
} process_t;

uint64_t create_user_address_space(uint64_t kernel_pml4_pa);
int map_user_stack(uint64_t pml4_pa, uint64_t stack_top, uint64_t size);
int alloc_thread_kernel_stack(thread_t* th, uint64_t kernel_pml4_pa);
void map_video_buffer(uint64_t pml4_pa);
static uint64_t g_next_kstack_slot;

#endif