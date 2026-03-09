#include <exception.h>
#include <printk.h>

void dump_registers(int_registers_t* regs) {
    printk("RIP: 0x%016lx ", regs->rip);
    printk("RSP: 0x%016lx\n", regs->rsp);
    printk("RFLAGS: 0x%016lx\n", regs->rflags);
    printk("RAX: 0x%016lx ", regs->rax);
    printk("RBX: 0x%016lx\n", regs->rbx);
    printk("RCX: 0x%016lx ", regs->rcx);
    printk("RDX: 0x%016lx\n", regs->rdx);
    printk("RSI: 0x%016lx ", regs->rsi);
    printk("RDI: 0x%016lx\n", regs->rdi);
    printk("RBP: 0x%016lx ", regs->rbp);
    printk("R8 : 0x%016lx\n", regs->r8);
    printk("R9 : 0x%016lx ", regs->r9);
    printk("R10: 0x%016lx\n", regs->r10);
    printk("R11: 0x%016lx ", regs->r11);
    printk("R12: 0x%016lx\n", regs->r12);
    printk("R13: 0x%016lx ", regs->r13);
    printk("R14: 0x%016lx\n", regs->r14);
    printk("R15: 0x%016lx\n", regs->r15);
}
void page_fault_handler(int_registers_t* regs) {
    uint64_t fault_addr;
    __asm__ volatile ("mov %%cr2, %0" : "=r"(fault_addr));
    printk("Page Fault at RIP: 0x%016lx, Faulting Address: 0x%016lx\n", regs->rip, fault_addr);
    printk("Error Code: 0x%016lx\n", regs->err_code);
    dump_registers(regs);
    while (1) { __asm__ volatile("hlt"); }
}   