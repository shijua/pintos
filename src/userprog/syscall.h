#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include "lib/kernel/hash.h"
#include "threads/thread.h"

#define STATUS_FAIL -1 /* Status code for syscall */
#define USER_BOTTOM 0x0804800 /* Bottom of user address */
#define GET_FILE(HASH_ELEM) hash_entry(HASH_ELEM, struct File_info, elem)
#define CHECK_NULL_FILE(file) file==NULL /* Check whether file is null */

typedef int pid_t;
#define PID_ERROR ((pid_t) -1)          /* Error value for pid_t. */

/* definition for arguments */
#define ARG_0 f->esp + 4
#define ARG_1 f->esp + 8
#define ARG_2 f->esp + 12

void syscall_init (void);
void syscall_exit (int);
unsigned file_hash_func (const struct hash_elem *element, void *aux UNUSED);
bool file_less_func (const struct hash_elem *a, const struct hash_elem *b, 
                                                void *aux UNUSED);
void free_struct_file (struct hash_elem *element, void *aux UNUSED);

/* File descriptor which store the file and the file descriptor number */
struct File_info {
  int fd;
  struct file *file;
  struct hash_elem elem;
};

#endif /* userprog/syscall.h */
