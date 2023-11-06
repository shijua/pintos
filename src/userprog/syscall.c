#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler (struct intr_frame *);
static void syscall_halt (void);
static void syscall_exit (int);
static void syscall_exec (const char *);
static void syscall_wait (pid_t);
static void syscall_create (const char *, unsigned);
static void syscall_remove (const char *);
static void syscall_open (const char *);
static void syscall_filesize (int);
static void syscall_read (int, void *, unsigned);
static void syscall_write (int, const void *, unsigned);
static void syscall_seek (int, unsigned);
static void syscall_tell (int);
static void syscall_close (int);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  // retrieve the system call number
  int syscall_num = *(int *) f->esp;
  printf ("system call number: %d\n", syscall_num);
  // switch on the system call number
  switch (syscall_num) {
    case SYS_HALT:
      syscall_halt ();
      break;
    case SYS_EXIT:
      syscall_exit (*(int *) (f->esp + 4));
      break;
    case SYS_EXEC:
      syscall_exec (*(char **) (f->esp + 4));
      break;
    case SYS_WAIT:
      syscall_wait (*(pid_t *) (f->esp + 4));
      break;
    case SYS_CREATE:
      syscall_create (*(char **) (f->esp + 4), *(unsigned *) (f->esp + 8));
      break;
    case SYS_REMOVE:
      syscall_remove (*(char **) (f->esp + 4));
      break;
    case SYS_OPEN:
      syscall_open (*(char **) (f->esp + 4));
      break;
    case SYS_FILESIZE:
      syscall_filesize (*(int *) (f->esp + 4));
      break;
    case SYS_READ:
      syscall_read (*(int *) (f->esp + 4), *(void **) (f->esp + 8), *(unsigned *) (f->esp + 12));
      break;
    case SYS_WRITE:
      syscall_write (*(int *) (f->esp + 4), *(void **) (f->esp + 8), *(unsigned *) (f->esp + 12));
      break;
    case SYS_SEEK:
      syscall_seek (*(int *) (f->esp + 4), *(unsigned *) (f->esp + 8));
      break;
    case SYS_TELL:
      syscall_tell (*(int *) (f->esp + 4));
      break;
    case SYS_CLOSE:
      syscall_close (*(int *) (f->esp + 4));
      break;
    default:
      break;
  }
}

/* Terminates Pintos (this should be seldom used). */
static void 
syscall_halt (void) {
  shutdown_power_off ();
}

/* Terminates the current user program, sending its 
   exit status to the kernal. */
static void 
syscall_exit (int status) {
  printf ("%s: exit(%d)\n", thread_current ()->name, status);
  process_exit ();
}

// TODO synchronisation is needed later
static void
syscall_exec (const char *cmd_line) {
  printf ("exec(%s)\n", cmd_line);
}

static void
syscall_wait (pid_t pid) {
  printf ("wait(%d)\n", pid);
}

static void
syscall_create (const char *file, unsigned initial_size) {
  printf ("create(%s, %d)\n", file, initial_size);
}

static void
syscall_remove (const char *file) {
  printf ("remove(%s)\n", file);
}

static void
syscall_open (const char *file) {
  printf ("open(%s)\n", file);
}

static void
syscall_filesize (int fd) {
  printf ("filesize(%d)\n", fd);
}

static void
syscall_read (int fd, void *buffer, unsigned size) {
  // printf ("read(%d, %p, %d)\n", fd, buffer, size);
}

static void
syscall_write (int fd, const void *buffer, unsigned size) {
  // pagedir_set_page(fd, buffer, buffer, true);
  /* Writes size bytes from buffer to the open file fd */
  // putbuf (buffer, size);
  printf ("write(%d, %p, %d)\n", fd, buffer, size);
}

static void
syscall_seek (int fd, unsigned position) {
  printf ("seek(%d, %d)\n", fd, position);
}

static void
syscall_tell (int fd) {
  printf ("tell(%d)\n", fd);
}

static void
syscall_close (int fd) {
  printf ("close(%d)\n", fd);
}