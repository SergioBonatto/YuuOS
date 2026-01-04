#pragma once

struct process *alloc_proc(int *pid_out);
struct process *create_process(const void *image, size_t image_size);
uint32_t *setup_proc_stack(struct process *proc);
void yield(void);

extern struct process procs[PROCS_MAX];
extern struct process *current_proc;
extern struct process *idle_proc;
