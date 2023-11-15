#include "userprog/syscall.h"
#include "userprog/pagedir.h"
#include <stdio.h>
#include <stdlib.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "devices/input.h"
#include "threads/synch.h"
#include "threads/vaddr.h"

static void syscall_handler(struct intr_frame *);
static void syscall_halt(void);
static pid_t syscall_exec(const char *);
static int syscall_wait(pid_t);
static int syscall_create(const char *, unsigned);
static int syscall_remove(const char *);
static int syscall_open(const char *);
static int syscall_filesize(int);
static int syscall_read(int, void *, unsigned);
static int syscall_write(int, const void *, unsigned);
static void syscall_seek(int, unsigned);
static void syscall_tell(int);
static void syscall_close(int);

/* Three functions used for checking user memory access safety. */
static void *check_validation(uint32_t *, const void *);
static void check_validation_str(const void *);
static void check_validation_rw(const void *, unsigned);

/* set a global lock for file system */
struct lock file_lock;
void
syscall_init(void) {
  intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&file_lock); // initialize the lock
}

static void
syscall_handler(struct intr_frame *f UNUSED) {

  // retrieve the system call number
  uint32_t cur_pd = thread_current ()->pagedir;
  int syscall_num = *((int *) check_validation (cur_pd, f->esp));

  // printf ("system call number: %d\n", syscall_num);
  int return_val;
  // switch on the system call number
  switch (syscall_num) {
    case SYS_HALT:
      syscall_halt();
      break;
    case SYS_EXIT:
      check_validation (cur_pd, f->esp+4);
      syscall_exit(*(int *) (f->esp + 4));
      break;
    case SYS_EXEC:
      check_validation (cur_pd, f->esp+4);
      check_validation_str (*(void **) (f->esp + 4));
      return_val = syscall_exec (*(char **) (f->esp + 4));
      f->eax = return_val;
      break;
    case SYS_WAIT:
      check_validation (cur_pd, f->esp+4);
      f->eax = syscall_wait (*(pid_t *) (f->esp + 4));
      break;
    case SYS_CREATE:
      check_validation (cur_pd, f->esp+4);
      check_validation (cur_pd, f->esp+8);
      check_validation_str (*(void **) (f->esp + 4));
      f->eax = syscall_create(*(char **) (f->esp + 4), *(unsigned *) (f->esp + 8));
      break;
    case SYS_REMOVE:
      check_validation (cur_pd, f->esp+4);
      check_validation_str (*(void **) (f->esp + 4));
      f->eax = syscall_remove(*(char **) (f->esp + 4));
      break;
    case SYS_OPEN:
      check_validation (cur_pd, f->esp+4);
      check_validation_str (*(void **) (f->esp + 4));
      f->eax = syscall_open(*(char **) (f->esp + 4));
      break;
    case SYS_FILESIZE:
      check_validation (cur_pd, f->esp+4);
      f->eax = syscall_filesize(*(int *) (f->esp + 4));
      break;
    case SYS_READ:
      check_validation (cur_pd, f->esp+4);
      check_validation_rw (*(void **) (f->esp + 8), *(void **) (f->esp + 12));
      f->eax = syscall_read(*(int *) (f->esp + 4), *(void **) (f->esp + 8), *(unsigned *) (f->esp + 12));
      break;
    case SYS_WRITE:
      check_validation (cur_pd, f->esp+4);
      check_validation_rw (*(void **) (f->esp + 8), *(void **) (f->esp + 12));
      f->eax = syscall_write(*(int *) (f->esp + 4), *(void **) (f->esp + 8), *(unsigned *) (f->esp + 12));
      break;
    case SYS_SEEK:
      check_validation (cur_pd, f->esp+4);
      check_validation (cur_pd, f->esp+8);
      syscall_seek(*(int *) (f->esp + 4), *(unsigned *) (f->esp + 8));
      break;
    case SYS_TELL:
      check_validation (cur_pd, f->esp+4);
      syscall_tell(*(int *) (f->esp + 4));
      break;
    case SYS_CLOSE:
      check_validation (cur_pd, f->esp+4);
      syscall_close(*(int *) (f->esp + 4));
      break;
    default:
      syscall_exit (STATUS_FAIL);
      break;
  }
}

/* Terminates Pintos (this should be seldom used). */
static void
syscall_halt(void) {
  shutdown_power_off();
}

/* Terminates the current user program, sending its 
   exit status to the kernal. */
void 
syscall_exit (int status) {
  // no need to uncomment this printf
  struct thread* cur = thread_current ();
  printf ("%s: exit(%d)\n", cur->name, status);
  // struct thread *cur = thread_current ();
  lock_acquire (&child_lock);
  if(cur -> wait_sema != NULL){
    *(cur->exit_code) = status;
    sema_up(cur->wait_sema);
  }
  lock_release (&child_lock);
  thread_exit ();
  NOT_REACHED ();
}

// TODO synchronisation is needed later
static pid_t
syscall_exec (const char *cmd_line) {
  // printf ("exec(%s)\n", cmd_line);
  pid_t pid = process_execute (cmd_line);
  return pid;
}

static int
syscall_wait (pid_t pid) {
  // printf ("wait(%d)\n", pid);
  return process_wait (pid);
}

static void
check_null_file(const char *file) {
  if (file == NULL) {
    lock_release(&file_lock); // release the lock
    syscall_exit(-1);
  }
}

// We ignore the synchronization problem for now.
static int
syscall_create(const char *file, unsigned initial_size) {
//  // printf("create(%s, %d)\n", file, initial_size);
  lock_acquire(&file_lock);
  check_null_file(file);
  bool success = filesys_create(file, initial_size);
  lock_release(&file_lock);
  return success;
}

static int
syscall_remove(const char *file) {
//  // printf("remove(%s)\n", file);
  lock_acquire(&file_lock);
  check_null_file(file);
  bool success = filesys_remove(file);
  lock_release(&file_lock);
  return success;
}

static int
syscall_open(const char *file) {
//  // printf ("open(%s)\n", file);
  lock_acquire(&file_lock);
  check_null_file(file);
  struct file *f = filesys_open(file);
  if (f == NULL) {
    return -1;
  } else {
    struct File_info *info = malloc(sizeof(struct File_info));
    info->fd = thread_current()->fd;
    info->file = f;
    list_push_back(&thread_current()->file_list, &info->elem);
  }
  lock_release(&file_lock);
  return thread_current()->fd++;
}

static struct File_info *
get_file_info(int fd) {
  struct list_elem *e;
  for (e = list_begin(&thread_current()->file_list);
       e != list_end(&thread_current()->file_list);
       e = list_next(e)) {
    struct File_info *info = list_entry(e,
    struct File_info, elem);
    if (info->fd == fd) {
      return info;
    }
  }
  return NULL; // If no file_info is found, return NULL
}

static int
syscall_filesize(int fd) {
//  // printf("filesize(%d)\n", fd);
  lock_acquire(&file_lock);
  struct File_info *info = get_file_info(fd);
  check_null_file(info->file);
  int size = file_length(info->file);
  lock_release(&file_lock);
  return size;
}

static int
syscall_read(int fd, void *buffer, unsigned size) {
//  // printf("read(%d, %s, %d)\n", fd, buffer, size);
  // Reads size bytes from the open file fd into buffer
  if (fd == 0) {
    // Standard input reading
    for (unsigned i = 0; i < size; i++) {
      ((char *) buffer)[i] = input_getc(); // It is a char!
    }
    return size;
  }
  lock_acquire(&file_lock);
  struct File_info *info = get_file_info(fd);
  check_null_file(info->file);
  int read_size = file_read(info->file, buffer, size);
  lock_release(&file_lock);
  return read_size;
}

static int
syscall_write(int fd, const void *buffer, unsigned size) {
//  // printf("write(%d, %s, %d)\n", fd, buffer, size);
  // Writes size bytes from buffer to the open file fd
  if (fd == 1) {
    // Standard output writing
    putbuf(buffer, size);//TODO it could be a big buffer
    return size;
  }
  lock_acquire(&file_lock);
  struct File_info *info = get_file_info(fd);
  check_null_file(info->file);
  int write_size = file_write(info->file, buffer, size);
  lock_release(&file_lock);
  return write_size;
}


static void
syscall_seek(int fd, unsigned position) {
//  // printf("seek(%d, %d)\n", fd, position);
  lock_acquire(&file_lock);
  struct File_info *info = get_file_info(fd);
  if (info) {
    file_seek(info->file, position);
  }
  lock_release(&file_lock);
}

static void
syscall_tell(int fd) {
//  // printf("tell(%d)\n", fd);
  lock_acquire(&file_lock);
  struct File_info *info = get_file_info(fd);
  if (info) {
    file_tell(info->file);
  }
  lock_release(&file_lock);
}

static void
syscall_close(int fd) {
//  // printf("close(%d)\n", fd);
  lock_acquire(&file_lock);
  struct File_info *info = get_file_info(fd);
  if (info) {
    file_close(info->file);
    list_remove(&info->elem);
    free(info);
  }
  lock_release(&file_lock);
}

void *
getpage_ptr(const void *vaddr) {
  void *ptr = pagedir_get_page (thread_current()->pagedir, vaddr);
  if (ptr == NULL) {
    syscall_exit (STATUS_FAIL);
  } else {
    return ptr;
  }
}

/* Function used for checking validation for the user virtual address is valid
   and returns the kernel virtual address from the specific address given. If
   the address given is invalid, call syscall_exit to terminate the process. */
static void *check_validation(uint32_t *pd, const void *vaddr) {
  // int pid_child_validation = process_wait (thread_current ()->tid);
  // if (pid_child_validation == -1) {
  //   syscall_exit (STATUS_FAIL);
  // }
  if (vaddr == NULL || !is_user_vaddr (vaddr)) {
    syscall_exit (STATUS_FAIL);
  } else {
    void *kernal_vaddr = pagedir_get_page (pd, vaddr);
    if (kernal_vaddr == NULL) {
      syscall_exit (STATUS_FAIL);
    } else {
      return kernal_vaddr;
    }
  }
}

/* Function used for making sure that the string is valid by using for loop. */
static void check_validation_str(const void * str) {
  if (str == NULL) {
    syscall_exit (STATUS_FAIL);
  }
  for (; *(char *) ((int) getpage_ptr(str)) != 0; str = (char *) str + 1);
}

/* Function used for making sure that the buffer stored the file is valid by
   using for loop.  */
static void check_validation_rw(const void *buffer, unsigned size) {
  unsigned index = 0;
  char *local = (char *) buffer;
  for (; index < size; index++) {
    if ((const void *) local < USER_BOTTOM
        || !is_user_vaddr ((const void *) local)) {
          syscall_exit (STATUS_FAIL);
        }
        local++;
  }
}
