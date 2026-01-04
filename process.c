#include "include/kernel.h"
#include "include/common.h"
#include "include/process.h"
#include "include/context.h"
#include "include/memory.h"



// find  an unused process control structure
struct process *alloc_proc(int *pid_out){
    for (int i = 0; i < PROCS_MAX; i++){
        if (procs[i].state == PROC_UNUSED) {
            *pid_out = i + 1;
            return &procs[i];
        }
    }
    return NULL;
}

// Stack callee-saved registers. These register values will be restored in
// the first context switch in switch_context.
uint32_t *setup_proc_stack(struct process *proc) {
    uint32_t *sp = (uint32_t *)&proc->stack[sizeof(proc->stack)];
    *--sp = 0;                         // s11
    *--sp = 0;                         // s10
    *--sp = 0;                         // s9
    *--sp = 0;                         // s8
    *--sp = 0;                         // s7
    *--sp = 0;                         // s6
    *--sp = 0;                         // s5
    *--sp = 0;                         // s4
    *--sp = 0;                         // s3
    *--sp = 0;                         // s2
    *--sp = 0;                         // s1
    *--sp = 0;                         // s0
    *--sp = (uint32_t) user_entry;  // ra

    return sp;
}


struct process *create_process(const void *image, size_t image_size){
    int pid;
    struct process *proc = alloc_proc(&pid);
    if (!proc)
        PANIC("no free process slots");

    // Initialize fields.
    proc->pid           = pid;
    proc->state         = PROC_RUNNABLE;
    proc->sp            = (uint32_t) setup_proc_stack(proc);
    proc->page_table    = create_user_pagetable(image, image_size);
    return proc;

}

void yield(void) {
    // search for a runnable process
    struct process *next = idle_proc;
    for (int i = 0; i < PROCS_MAX; i++) {
        struct process *proc = &procs[(current_proc->pid + i) % PROCS_MAX];
        if (proc->state == PROC_RUNNABLE && proc->pid > 0){
            next = proc;
            break;
        }
    }

    // if there's no runnable process other than the current one
    // return and continue processing
    if (next == current_proc)
        return;

    __asm__ __volatile__(
        "sfence.vma\n"
        "csrw satp, %[satp]\n"
        "sfence.vma\n"
        "csrw sscratch, %[sscratch]\n"
        :
        :   [satp] "r" (SATP_SV32 | ((uint32_t) next->page_table / PAGE_SIZE)),
            [sscratch] "r" ((uint32_t) &next->stack[sizeof(next->stack)])
    );


    // context switch
    struct process *prev = current_proc;
    current_proc = next;
    switch_context(&prev->sp, &next->sp);
}
