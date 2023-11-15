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
#include "threads/malloc.h"
#include "devices/shutdown.h"
#include "userprog/process.h"

static void syscall_handler (struct intr_frame *);
static void syscall_halt (void);
static pid_t syscall_exec (const char *);
static int syscall_wait (pid_t);
static int syscall_create (const char *, unsigned);
static int syscall_remove (const char *);
static int syscall_open (const char *);
static int syscall_filesize (int);
static int syscall_read (int, void *, unsigned);
static int syscall_write (int, void *, unsigned);
static void syscall_seek (int, unsigned);
static void syscall_tell (int);
static void syscall_close (int);

static void check_null_file (struct file *file);
static struct File_info *get_file_info (int fd);

void *getpage_ptr(const void *vaddr);
/* Three functions used for checking user memory access safety. */
static void *check_validation (const void *);
static void check_validation_str (const void *);
static void check_validation_rw (const void *, unsigned);

/* init the syscall */
void
syscall_init (void) {
  intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
}

/* direct to related system call according to system call number */
static void
syscall_handler (struct intr_frame *f UNUSED) {
  // retrieve the system call number
  int syscall_num = *((int *) check_validation (f->esp));

  switch (syscall_num) {
    case SYS_HALT:
      syscall_halt();
      break;
    case SYS_EXIT:
      check_validation (ARG_0);
      syscall_exit(*(int *) (ARG_0));
      break;
    case SYS_EXEC:
      check_validation (ARG_0);
      check_validation_str (*(void **) (ARG_0));
      f->eax = syscall_exec (*(char **) (ARG_0));
      break;
    case SYS_WAIT:
      check_validation (ARG_0);
      f->eax = syscall_wait (*(pid_t *) (ARG_0));
      break;
    case SYS_CREATE:
      check_validation (ARG_0);
      check_validation (ARG_1);
      check_validation_str (*(void **) (ARG_0));
      f->eax = syscall_create(*(char **) (ARG_0), *(unsigned *) (ARG_1));
      break;
    case SYS_REMOVE:
      check_validation (ARG_0);
      check_validation_str (*(void **) (ARG_0));
      f->eax = syscall_remove(*(char **) (ARG_0));
      break;
    case SYS_OPEN:
      check_validation (ARG_0);
      check_validation_str (*(void **) (ARG_0));
      f->eax = syscall_open (*(char **) (ARG_0));
      break;
    case SYS_FILESIZE:
      check_validation (ARG_0);
      f->eax = syscall_filesize (*(int *) (ARG_0));
      break;
    case SYS_READ:
      check_validation (ARG_0);
      check_validation_rw (*(void **) (ARG_1), *(unsigned *) (ARG_2));
      f->eax = syscall_read (*(int *) (ARG_0), *(void **) (ARG_1), 
                                               *(unsigned *) (ARG_2));
      break;
    case SYS_WRITE:
      check_validation (ARG_0);
      check_validation_rw (*(void **) (ARG_1), *(unsigned *) (ARG_2));
      f->eax = syscall_write (*(int *) (ARG_0), *(void **) (ARG_1), 
                                                *(unsigned *) (ARG_2));
      break;
    case SYS_SEEK:
      check_validation (ARG_0);
      check_validation (ARG_1);
      syscall_seek (*(int *) (ARG_0), *(unsigned *) (ARG_1));
      break;
    case SYS_TELL:
      check_validation (ARG_0);
      syscall_tell (*(int *) (ARG_0));
      break;
    case SYS_CLOSE:
      check_validation (ARG_0);
      syscall_close (*(int *) (ARG_0));
      break;
    default:
      syscall_exit (STATUS_FAIL);
      break;
  }
}

/* Terminates Pintos (this should be seldom used). */
static void
syscall_halt (void) {
  shutdown_power_off();
}

/* Terminates the current user program, sending its 
   exit status to the kernal. */
void 
syscall_exit (int status) {
  struct thread* cur = thread_current ();
  printf ("%s: exit(%d)\n", cur->name, status);
  *(cur->exit_code) = status;
  sema_up (cur->wait_sema);
  thread_exit ();
  NOT_REACHED ();
}

/* Runs the executable whose name is given in cmd line, passing any given 
   arguments, and returns the new process’s program id (pid). */
static pid_t
syscall_exec (const char *cmd_line) {
  return process_execute (cmd_line);
}

/* Waits for a child process pid and retrieves the child’s exit status. */
static int
syscall_wait (pid_t pid) {
  return process_wait (pid);
}

/* Creates a new ﬁle called ﬁle initially initial size bytes in size. */
static int
syscall_create (const char *file, unsigned initial_size) {
  lock_acquire (&file_lock);
  bool success = filesys_create (file, initial_size);
  lock_release (&file_lock);
  return success;
}

/* Deletes the ﬁle called ﬁle. Returns true if successful, false otherwise. */
static int
syscall_remove (const char *file) {
  lock_acquire (&file_lock);
  bool success = filesys_remove (file);
  lock_release (&file_lock);
  return success;
}

/* Opens the ﬁle called ﬁle. Returns a nonnegative integer handle called a 
  “ﬁle descriptor” (fd), or -1 if the ﬁle could not be opened. */
static int
syscall_open (const char *file) {
  lock_acquire (&file_lock);
  struct file *f = filesys_open (file);
  if (f == NULL) {
    return -1;
  } else {
    struct File_info *info = malloc (sizeof (struct File_info));
    info->fd = thread_current ()->fd;
    info->file = f;
    list_push_back (&thread_current ()->file_list, &info->elem);
  }
  lock_release (&file_lock);
  return thread_current ()->fd++;
}

/* Returns the size, in bytes, of the ﬁle open as fd. */
static int
syscall_filesize (int fd) {
  lock_acquire (&file_lock);
  struct File_info *info = get_file_info( fd);
  check_null_file (info->file);
  int size = file_length (info->file);
  lock_release (&file_lock);
  return size;
}

/* Reads size bytes from the ﬁle open as fd into buﬀer. */
static int
syscall_read (int fd, void *buffer, unsigned size) {
  // Reads size bytes from the open file fd into buffer
  if (fd == 0) {
    // Standard input reading
    for (unsigned i = 0; i < size; i++) {
      ((char *) buffer)[i] = input_getc (); // It is a char!
    }
    return size;
  }
  lock_acquire (&file_lock);
  struct File_info *info = get_file_info (fd);
  check_null_file (info->file);
  int read_size = file_read (info->file, buffer, size);
  lock_release (&file_lock);
  return read_size;
}

/* Writes size bytes from buﬀer to the open ﬁle fd. */
static int
syscall_write (int fd, void *buffer, unsigned size) {
  // Writes size bytes from buffer to the open file fd
  if (fd == 1) {
    // Standard output writing
    putbuf (buffer, size);
    return size;
  }
  lock_acquire (&file_lock);
  struct File_info *info = get_file_info (fd);
  check_null_file (info->file);
  int write_size = file_write (info->file, buffer, size);
  lock_release (&file_lock);
  return write_size;
}

/* Changes the next byte to be read or written in open ﬁle fd to position, 
   expressed in bytes from the beginning of the ﬁle.*/
static void
syscall_seek (int fd, unsigned position) {
  lock_acquire (&file_lock);
  struct File_info *info = get_file_info (fd);
  if (info) {
    file_seek (info->file, position);
  }
  lock_release (&file_lock);
}

/* Returns the position of the next byte to be read or written in open ﬁle fd, 
   expressed in bytes from the beginning of the ﬁle. */
static void
syscall_tell (int fd) {
  lock_acquire (&file_lock);
  struct File_info *info = get_file_info(fd);
  if (info) {
    file_tell (info->file);
  }
  lock_release (&file_lock);
}

/* Closes ﬁle descriptor fd. Exiting or terminating a process implicitly closes 
   all its open ﬁle descriptors, as if by calling this function for each one. */
static void
syscall_close (int fd) {
  lock_acquire(&file_lock);
  struct File_info *info = get_file_info(fd);
  if (info) {
    file_close (info->file);
    list_remove (&info->elem);
    free (info);
  }
  lock_release (&file_lock);
}

/* use for checking whether file retained is null or not */
static void
check_null_file (struct file *file) {
  if (file == NULL) {
    lock_release (&file_lock); // release the lock
    syscall_exit (-1);
  }
}

/* get file info from fd */
static struct File_info *
get_file_info (int fd) {
  struct list_elem *e;
  for (e = list_begin (&thread_current ()->file_list);
       e != list_end (&thread_current ()->file_list);
       e = list_next (e)) {
    struct File_info *info = list_entry(e,
    struct File_info, elem);
    if (info->fd == fd) {
      return info;
    }
  }
  // If no file_info is found, return NULL
  return NULL; 
}

void *
getpage_ptr(const void *vaddr) {
  void *ptr = pagedir_get_page (thread_current ()->pagedir, vaddr);
  if (ptr == NULL) {
    syscall_exit (STATUS_FAIL);
  } else {
    return ptr;
  }
  NOT_REACHED ();
  return NULL;
}

/* Function used for checking validation for the user virtual address is valid
   and returns the kernel virtual address from the specific address given. If
   the address given is invalid, call syscall_exit to terminate the process. */
static void *check_validation(const void *vaddr) {
  uint32_t *pd = thread_current ()->pagedir;
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
  NOT_REACHED ();
  return NULL;
}

/* Function used for making sure that the string is valid by using for loop. */
static void check_validation_str (const void * str) {
  if (str == NULL) {
    syscall_exit (STATUS_FAIL);
  }
  for (; *(char *) ((int) getpage_ptr (str)) != 0; str = (char *) str + 1);
}

/* Function used for making sure that the buffer stored the file is valid by
   using for loop.  */
static void check_validation_rw (const void *buffer, unsigned size) {
  // address of the start of buffer
  uint32_t local = (uint32_t) buffer;
  unsigned buffer_length = local + size;
  for (; local < buffer_length; local++) {
    if (local < USER_BOTTOM || !is_user_vaddr ((const void *) local)) {
          syscall_exit (STATUS_FAIL);
        }
  }
}
