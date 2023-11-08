#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include "lib/kernel/list.h"

#define STATUS_SUCC 0
#define STATUS_FAIL -1

/* Process identifier. */
typedef int pid_t;
#define PID_ERROR ((pid_t) -1)

void syscall_init(void);

/* File descriptor which store the file and the file descriptor number */
struct File_info {
  int fd;
  struct file *file;
  struct list_elem elem;
};

//struct lock syscall_lock;

#endif /* userprog/syscall.h */
