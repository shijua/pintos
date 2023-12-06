#ifndef USERPROG_EXCEPTION_H
#define USERPROG_EXCEPTION_H

#include <stdbool.h>
#include "vm/pageTable.h"
/* Page fault error code bits that describe the cause of the exception.  */
#define PF_P 0x1    /* 0: not-present page. 1: access rights violation. */
#define PF_W 0x2    /* 0: read, 1: write. */
#define PF_U 0x4    /* 0: kernel, 1: user process. */
#define PUSH_SIZE 4
#define PUSH_A_SIZE 32
#define STACK_MAX 0x800000

void load_page(struct lazy_file *Lfile, struct page_elem *page);
void exception_init (void);
void exception_print_stats (void);
bool is_stack_address (void *fault_addr, void *esp);
#endif /* userprog/exception.h */
