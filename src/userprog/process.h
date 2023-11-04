#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

#define getParameter(LIST_ELEM) list_entry(LIST_ELEM, struct parameterValue,elem)

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

struct parameterValue{
    char *data;
    uint32_t address;
    struct list_elem elem;
};

#endif /* userprog/process.h */
