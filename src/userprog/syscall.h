#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#define STATUS_SUCC 0
#define STATUS_FAIL -1

/* Process identifier. */
typedef int pid_t;
#define PID_ERROR ((pid_t) -1)

void syscall_init (void);

#endif /* userprog/syscall.h */
