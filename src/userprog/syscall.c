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
#include <inttypes.h>
#include "vm/pageTable.h"
#include "vm/frame.h"
#include "threads/palloc.h"
#include "userprog/exception.h"

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
static void mmap(struct intr_frame *f);
static void unmmap(struct intr_frame *f);

static int mmapInt = 0;

static struct File_info *get_file_info (int fd);

/* Three functions used for checking user memory access safety. */
static void check_validation (const void *);
static void check_validation_rw (const void *, unsigned);
static void check_validation_str (const char **vaddr);

/* function used for pin and unpin frame */
static void pin_frame (void *uaddr);
static void unpin_frame (void *uaddr);

unsigned 
file_hash_func (const struct hash_elem *element, void *aux UNUSED) 
{
  return(hash_int(GET_FILE(element)->fd));
}

bool 
file_less_func (const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED)
{
  return(GET_FILE(a)->fd < GET_FILE(b)->fd);
}

unsigned 
mmap_hash_func (const struct hash_elem *element, void *aux UNUSED) 
{
  return(hash_int(hash_entry(element, struct mmapElem, elem)->mapid));
}

bool 
mmap_less_func (const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED)
{
  return(hash_entry(a, struct mmapElem, elem)->mapid
    < hash_entry(b, struct mmapElem, elem)->mapid);
}


void free_struct_file (struct hash_elem *element, void *aux UNUSED)
{
  struct File_info *info = GET_FILE(element);
  file_close (info->file);
  free (info);
}

void free_struct_mmap (struct hash_elem *element, void *aux UNUSED)
{
  struct mmapElem *info = hash_entry(element, struct mmapElem, elem);
  free (info);
}


/* init the syscall */
void
syscall_init (void) {
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

/* direct to related system call according to system call number */
static void
syscall_handler (struct intr_frame *f UNUSED) {
  /* store esp to the current thread */
  thread_current ()->esp = f->esp;
  /* retrieve the system call number */
  check_validation (f->esp);
  int syscall_num = *((int *) f->esp);
  switch (syscall_num) {
    case SYS_HALT:
      syscall_halt();
      break;
    case SYS_EXIT:
      check_validation (ARG_0);
      syscall_exit (*(int *) (ARG_0));
      break;
    case SYS_EXEC:
      check_validation_str (ARG_0);
      f->eax = syscall_exec (*(char **) (ARG_0));
      break;
    case SYS_WAIT:
      check_validation (ARG_0);
      f->eax = syscall_wait (*(pid_t *) (ARG_0));
      break;
    case SYS_CREATE:
      check_validation_str (ARG_0);
      check_validation (ARG_1);
      f->eax = syscall_create (*(char **) (ARG_0), *(unsigned *) (ARG_1));
      break;
    case SYS_REMOVE:
      check_validation_str (ARG_0);
      f->eax = syscall_remove (*(char **) (ARG_0));
      break;
    case SYS_OPEN:
      check_validation_str (ARG_0);
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
    case SYS_MMAP:
      mmap(f);
      break;
    case SYS_MUNMAP:
      unmmap(f);
      break;
    default:
      syscall_exit (STATUS_FAIL);
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
void 
syscall_exit (int status) {
  struct thread* cur = thread_current ();
  printf ("%s: exit(%"PRId32")\n", cur->name, status);
  /* ensure the file lock has been released */
  if (file_lock.holder == cur) {
    lock_release (&file_lock);
  }
  if (cur -> parent_status == false && cur -> child_status_pointer != NULL) {
    *(cur -> child_status_pointer) = true;
    *(cur->exit_code) = status;
    sema_up (cur->wait_sema);
  }
  thread_exit ();
  NOT_REACHED ();
}

/* Runs the executable whose name is given in cmd line, passing any given 
   arguments, and returns the new process’s program id (pid). */
static pid_t
syscall_exec (const char *cmd_line) {
  pin_frame ((void*) cmd_line);
  pid_t pid = process_execute (cmd_line);
  unpin_frame ((void*) cmd_line);
  return pid;
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
  pin_frame (file);
  bool success = filesys_create (file, initial_size);
  unpin_frame (file);
  lock_release (&file_lock);
  return success;
}

/* Deletes the ﬁle called ﬁle. Returns true if successful, false otherwise. */
static int
syscall_remove (const char *file) {
  lock_acquire (&file_lock);
  pin_frame (file);
  bool success = filesys_remove (file);
  unpin_frame (file);
  lock_release (&file_lock);
  return success;
}

/* Opens the ﬁle called ﬁle. Returns a nonnegative integer handle called a 
  “ﬁle descriptor” (fd), or -1 if the ﬁle could not be opened. */
static int
syscall_open (const char *file) {
  lock_acquire (&file_lock);
  pin_frame (file);
  struct file *f = filesys_open (file);
  if (f == NULL) {
    lock_release (&file_lock);
    return -1;
  } else {
    struct File_info *info = malloc (sizeof (struct File_info));
    if (info == NULL) {
      unpin_frame (file);
      lock_release (&file_lock);
      syscall_exit (STATUS_FAIL);
    }
    info->fd = thread_current ()->fd;
    info->file = f;
    hash_insert(&thread_current ()->file_table, &info->elem);
  }
  unpin_frame (file);
  lock_release (&file_lock);
  return thread_current ()->fd++;
}

/* Returns the size, in bytes, of the ﬁle open as fd. */
static int
syscall_filesize (int fd) {
  lock_acquire (&file_lock);
  struct File_info *info = get_file_info (fd);
  if (CHECK_NULL_FILE (info->file)) {
    lock_release (&file_lock);
    syscall_exit (STATUS_FAIL);
  }
  int size = file_length (info->file);
  lock_release (&file_lock);
  return size;
}

/* Reads size bytes from the ﬁle open as fd into buﬀer. */
static int
syscall_read (int fd, void *buffer, unsigned size) {
  /* Reads size bytes from the open file fd into buffer */
  pin_frame (buffer);
  if (fd == 0) {
    /* Standard input reading */
    for (unsigned i = 0; i < size; i++) {
      ((char *) buffer)[i] = input_getc (); // It is a char!
    }
    return size;
  }
  lock_acquire (&file_lock);
  struct File_info *info = get_file_info (fd);
  if (CHECK_NULL_FILE (info->file)) {
    unpin_frame (buffer);
    lock_release (&file_lock);
    syscall_exit (STATUS_FAIL);
  }
  int read_size = file_read (info->file, buffer, size);
  unpin_frame (buffer);
  lock_release (&file_lock);
  return read_size;
}

/* Writes size bytes from buﬀer to the open ﬁle fd. */
static int
syscall_write (int fd, void *buffer, unsigned size) {
  /* Writes size bytes from buffer to the open file fd */
  pin_frame (buffer);
  if (fd == 1) {
    /* Standard output writing */
    putbuf (buffer, size);
    return size;
  }
  lock_acquire (&file_lock);
  struct File_info *info = get_file_info (fd);
  if (CHECK_NULL_FILE (info->file)) {
    unpin_frame (buffer);
    lock_release (&file_lock);
    syscall_exit (STATUS_FAIL);
  }
  int write_size = file_write (info->file, buffer, size);
  unpin_frame (buffer);
  lock_release (&file_lock);
  return write_size;
}

/* Changes the next byte to be read or written in open ﬁle fd to position, 
   expressed in bytes from the beginning of the ﬁle. */
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
  lock_acquire (&file_lock);
  struct File_info *info = get_file_info (fd);
  if (info) {
    file_close (info->file);
    hash_delete (&thread_current()->file_table, &info->elem);
    free (info);
  }
  lock_release (&file_lock);
}

/* get file info from fd */
static struct File_info *
get_file_info (int fd) {
  struct File_info key;
  key.fd = fd; 
  struct hash_elem *e = hash_find(&thread_current()->file_table, &key.elem);
  if(e != NULL){
     return GET_FILE(e);
  }
  /* If no file_info is found, return NULL */
  return NULL; 
}

/* Function used for checking validation for the user virtual address is valid
   and check kernel virtual address from the specific address given. If
   the address given is invalid, call syscall_exit to terminate the process. */
static void check_validation (const void *vaddr) {
  uint32_t *pd = thread_current ()->pagedir;
  if (vaddr == NULL || !is_user_vaddr (vaddr)) {
    syscall_exit (STATUS_FAIL);
  } else {
    page_elem page_elem = pageLookUp (pg_round_down (vaddr));
    if (page_elem == NULL) {
      syscall_exit (STATUS_FAIL);
    } 
  }
}

/* special case for string that need to check whether content is null */
static void check_validation_str (const char **vaddr) {
  if (*vaddr == NULL) {
    syscall_exit (STATUS_FAIL);
  }
  check_validation (vaddr);
}

/* Function used for making sure that the buffer stored the file is valid by
   using for loop.  */
static void check_validation_rw (const void *buffer, unsigned size) {
  /* address of the start of buffer */
  uint32_t local = (uint32_t) pg_round_down (buffer);
  unsigned buffer_length = local + size;
  for (; local < buffer_length; local += PGSIZE) {
    if (local < USER_BOTTOM || !is_user_vaddr ((const void *) local)) {
          syscall_exit (STATUS_FAIL);
    }
  }
  if (buffer_length < USER_BOTTOM || !is_user_vaddr ((const void *) buffer_length)) {
    syscall_exit (STATUS_FAIL);
  }
  check_validation (buffer);
}

static void mmap(struct intr_frame *f){
  int fd = *(int *)(f->esp + 4);
  uint8_t *address = *(uint8_t **)(f->esp + 8);
  struct File_info *find = get_file_info(fd);
  if(find == NULL){
    f->eax = -1;
    return;
  }
  // TODO doing sync check
  lock_acquire(&file_lock);
  struct file *file = file_reopen(find -> file);
  lock_release(&file_lock);
  struct mmapElem *adding = malloc(sizeof(struct mmapElem));
  if(is_stack_address(address, f->esp) || !load_mmap(file, address, adding)){
    free(adding);
    f->eax = -1;
    return;
  }
  f->eax = mmapInt;
  adding->file = file;
  adding->mapid = mmapInt++;
  adding->page_address = address;
  hash_insert(&thread_current()->mmap_hash, &adding->elem);
}

void
munmapHelper(struct hash_elem *found_elem, void *aux UNUSED)
{
  struct mmapElem *found = hash_entry(found_elem, struct mmapElem, elem);
  int n = found->page_num;
  for(int i = 0; i < n; i++){
    uint8_t *page = found->page_address + i*PGSIZE;
    if(pagedir_get_page(thread_current()->pagedir, page) != NULL
      && pagedir_is_dirty(thread_current()->pagedir, page)) {
        lock_acquire(&file_lock);
        int k = file_write_at(found->file, page, PGSIZE, i*PGSIZE);
        lock_release(&file_lock);
    }
    
    pagedir_clear_page(thread_current()->pagedir, page);
    page_clear(page);
  }
  lock_acquire(&file_lock);
  file_close(found->file);
  lock_release(&file_lock);
  free(found);
}

static void unmmap(struct intr_frame *f)
{
  struct mmapElem temp;
  temp.mapid = *(int *)(f->esp + 4);
  struct hash_elem *find = hash_find(&thread_current()->mmap_hash, &temp.elem);
  if(find == NULL) {
    PANIC("mapid not found");
  }
  hash_delete(&thread_current()->mmap_hash, find);
  munmapHelper(find, NULL);
}

static void pin_frame (void *uaddr) {
  lock_acquire(&thread_current()->page_lock);
  if (!page_set_pin ((uint32_t) pg_round_down (uaddr), true)) {
    lock_release(&thread_current()->page_lock);
    syscall_exit (STATUS_FAIL);
  }
  lock_release(&thread_current()->page_lock);
}

static void unpin_frame (void *uaddr) {
  lock_acquire(&thread_current()->page_lock);
  if (!page_set_pin ((uint32_t) pg_round_down (uaddr), false)) {
    lock_release(&thread_current()->page_lock);
    syscall_exit (STATUS_FAIL);
  }
  lock_release(&thread_current()->page_lock);
}