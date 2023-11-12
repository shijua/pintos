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
static void syscall_exit(int);
static pid_t syscall_exec(const char *);
static int syscall_wait(pid_t);
static bool syscall_create(const char *, unsigned);
static bool syscall_remove(const char *);
static int syscall_open(const char *);
static int syscall_filesize(int);
static int syscall_read(int, void *, unsigned);
static int syscall_write(int, const void *, unsigned);
static void syscall_seek(int, unsigned);
static void syscall_tell(int);
static void syscall_close(int);

static void *check_validation(uint32_t *, const void *);

/* set a global lock for file system */
struct lock file_lock;
void
syscall_init(void) {
  intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&file_lock); // initialize the lock
}

static void
syscall_handler(struct intr_frame *f UNUSED) {

//  lock_init (&syscall_lock);

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
      syscall_exit(*(int *) (f->esp + 4));
      break;
    case SYS_EXEC:
      return_val = syscall_exec (*(char **) (f->esp + 4));
      if (return_val != -1) {
          f->eax = return_val;
      }
      break;
    case SYS_WAIT:
      syscall_wait (*(pid_t *) (f->esp + 4));
      break;
    case SYS_CREATE:
      syscall_create(*(char **) (f->esp + 4), *(unsigned *) (f->esp + 8));
      break;
    case SYS_REMOVE:
      syscall_remove(*(char **) (f->esp + 4));
      break;
    case SYS_OPEN:
      syscall_open(*(char **) (f->esp + 4));
      break;
    case SYS_FILESIZE:
      syscall_filesize(*(int *) (f->esp + 4));
      break;
    case SYS_READ:
      syscall_read(*(int *) (f->esp + 4), *(void **) (f->esp + 8), *(unsigned *) (f->esp + 12));
      break;
    case SYS_WRITE:
      syscall_write(*(int *) (f->esp + 4), *(void **) (f->esp + 8), *(unsigned *) (f->esp + 12));
      break;
    case SYS_SEEK:
      syscall_seek(*(int *) (f->esp + 4), *(unsigned *) (f->esp + 8));
      break;
    case SYS_TELL:
      syscall_tell(*(int *) (f->esp + 4));
      break;
    case SYS_CLOSE:
      syscall_close(*(int *) (f->esp + 4));
      break;
    default:
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
static void 
syscall_exit (int status) {
  // no need to uncomment this printf
  printf ("%s: exit(%d)\n", thread_current ()->name, status);
  // struct thread *cur = thread_current ();
  lock_acquire (&child_lock);
  list_remove (&thread_current ()->child_elem);
  lock_release (&child_lock);
  thread_exit ();
  NOT_REACHED ();
}

// TODO synchronisation is needed later
static pid_t
syscall_exec (const char *cmd_line) {
  // printf ("exec(%s)\n", cmd_line);
  enum intr_level old_level = intr_disable ();
  lock_acquire (&child_lock);
  pid_t pid = process_execute (cmd_line);
  lock_release (&child_lock);
  intr_set_level (old_level);
  return pid;
}

static int
syscall_wait (pid_t pid) {
  // printf ("wait(%d)\n", pid);
  return process_wait (pid);
}

// We ignore the synchronization problem for now.
static bool
syscall_create(const char *file, unsigned initial_size) {
//  // printf("create(%s, %d)\n", file, initial_size);
  lock_acquire(&file_lock);
  bool success = filesys_create(file, initial_size);
  lock_release(&file_lock);
  return success;
}

static bool
syscall_remove(const char *file) {
//  // printf("remove(%s)\n", file);
  lock_acquire(&file_lock);
  bool success = filesys_remove(file);
  lock_release(&file_lock);
  return success;
}

static int
syscall_open(const char *file) {
//  // printf ("open(%s)\n", file);
  lock_acquire(&file_lock);
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
  if (info) {
    return file_length(info->file);
  }
  lock_release(&file_lock);
  return -1;
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
  if (info) {
    return file_read(info->file, buffer, size);
  }
  lock_release(&file_lock);
  return -1;
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
  if (info) {
    return file_write(info->file, buffer, size);
  }
  lock_release(&file_lock);
  return -1;
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

static void *check_validation(uint32_t *pd, const void *vaddr) {
  void *kernal_vaddr = pagedir_get_page (pd, vaddr);
  if (vaddr == NULL || !is_user_vaddr (vaddr)) {
    thread_exit ();
  } else {
    if (kernal_vaddr == NULL) {
      thread_exit ();
    } else {
      return kernal_vaddr;
    }
  }
}