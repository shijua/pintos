#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include "lib/kernel/list.h"
#include "threads/thread.h"

#define STATUS_FAIL -1 /* Status code for syscall */
#define USER_BOTTOM 0x0804800 /* Bottom of user address */

typedef int pid_t;
#define PID_ERROR ((pid_t) -1)          /* Error value for pid_t. */

/* definition for arguments */
#define ARG_0 f->esp + 4
#define ARG_1 f->esp + 8
#define ARG_2 f->esp + 12

void syscall_init (void);
void syscall_exit (int);

/* File descriptor which store the file and the file descriptor number */
struct File_info {
  int fd;
  struct file *file;
  struct list_elem elem;
};

#endif /* userprog/syscall.h */
