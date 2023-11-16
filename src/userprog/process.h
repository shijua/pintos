#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
#include "syscall.h"

/* shortcur to express list_entry*/
#define GET_PARAMETER(LIST_ELEM) list_entry \
    (LIST_ELEM, struct parameter_value, elem)

/* Coefficient for stack overflow. */
#define PTR_SIZE 4
#define STRING_BLANK 1 
#define STACK_BASE 16 

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

/* structure to express the content in parameter */
struct parameter_value {
    char *data;
    uint32_t address;
    struct list_elem elem;
};

#endif /* userprog/process.h */
