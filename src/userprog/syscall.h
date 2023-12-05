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

/* File descriptor which store the file and the file descriptor number */
struct File_info {
  int fd;
  struct file *file;
  struct hash_elem elem;
};

struct mmapElem{
 int mapid;
 struct hash_elem elem;
 uint32_t page_num;
 uint32_t page_address;
 struct file *file;   
};

void syscall_init (void);
void syscall_exit (int);
void munmapHelper(struct hash_elem *, void *);
hash_hash_func file_hash_func;
hash_less_func file_less_func;
hash_hash_func mmap_hash_func;
hash_less_func mmap_less_func;
hash_action_func free_struct_file;
hash_action_func free_struct_mmap;

#endif /* userprog/syscall.h */
