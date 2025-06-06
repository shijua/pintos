#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
#include "syscall.h"

/* Coefficient for stack overflow. */
#define PTR_SIZE sizeof(void *) // 4
#define STRING_BLANK 1
#define STACK_BASE 16

tid_t process_execute(const char *file_name);
int process_wait(tid_t);
void process_exit(void);
void process_activate(void);

/* load() helpers. */
bool install_page(void *upage, void *kpage, bool writable);
#endif /* userprog/process.h */
