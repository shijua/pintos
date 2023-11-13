#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include "lib/kernel/list.h"

#define STATUS_SUCC 0
#define STATUS_FAIL -1
#include "threads/thread.h"
#include <list.h>
/* Process identifier. */
// typedef struct thread* pid_t;
// #define PID_ERROR NULL

typedef int pid_t;
#define PID_ERROR ((pid_t) -1)          /* Error value for pid_t. */

void syscall_init(void);
void syscall_exit (int);

/* File descriptor which store the file and the file descriptor number */
struct File_info {
  int fd;
  struct file *file;
  struct list_elem elem;
};

//struct lock syscall_lock;

#endif /* userprog/syscall.h */
