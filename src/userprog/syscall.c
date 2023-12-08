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

static void syscall_handler(struct intr_frame *f);
static void syscall_halt(struct intr_frame *f);
static void syscall_exit(struct intr_frame *f);
static void syscall_exec(struct intr_frame *f);
static void syscall_wait(struct intr_frame *f);
static void syscall_create(struct intr_frame *f);
static void syscall_remove(struct intr_frame *f);
static void syscall_open(struct intr_frame *f);
static void syscall_filesize(struct intr_frame *f);
static void syscall_read(struct intr_frame *f);
static void syscall_write(struct intr_frame *f);
static void syscall_seek(struct intr_frame *f);
static void syscall_tell(struct intr_frame *f);
static void syscall_close(struct intr_frame *f);
static void syscall_mmap(struct intr_frame *f);
static void syscall_unmmap(struct intr_frame *f);

static void (*fun_ptr_arr[])(struct intr_frame *f) =
    {
        syscall_halt, syscall_exit, syscall_exec, syscall_wait, syscall_create,
        syscall_remove, syscall_open, syscall_filesize, syscall_read, syscall_write,
        syscall_seek, syscall_tell, syscall_close, syscall_mmap, syscall_unmmap};

static struct File_info *get_file_info(int fd);

/* Three functions used for checking user memory access safety. */
static void check_validation(void *);
static void check_validation_rw(void *, unsigned);
static void check_validation_str(char **vaddr);

/* function used for pin and unpin frame */
static void pin_frame(void *uaddr);
static void unpin_frame(void *uaddr);
static void unpin_frame_file(void *uaddr, int size);

/* function used on mmap and unmmap */
static bool load_mmap(struct file *file, uint32_t upage, struct mmap_elem *mmap_elem);

unsigned
file_hash_func(const struct hash_elem *element, void *aux UNUSED)
{
  return (hash_int(GET_FILE(element)->fd));
}

bool file_less_func(const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED)
{
  return (GET_FILE(a)->fd < GET_FILE(b)->fd);
}

unsigned
mmap_hash_func(const struct hash_elem *element, void *aux UNUSED)
{
  return (hash_int(hash_entry(element, struct mmap_elem, elem)->mapid));
}

bool mmap_less_func(const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED)
{
  return (hash_entry(a, struct mmap_elem, elem)->mapid < hash_entry(b, struct mmap_elem, elem)->mapid);
}

void free_struct_file(struct hash_elem *element, void *aux UNUSED)
{
  struct File_info *info = GET_FILE(element);
  file_close(info->file);
  free(info);
}

void free_struct_mmap(struct hash_elem *element, void *aux UNUSED)
{
  struct mmap_elem *info = hash_entry(element, struct mmap_elem, elem);
  free(info);
}

/* init the syscall */
void syscall_init(void)
{
  intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
}

/* direct to related system call according to system call number */
static void
syscall_handler(struct intr_frame *f UNUSED)
{
  /* retrieve the system call number */
  check_validation(ESP);
  uint32_t syscall_num = *((int *)ESP);
  unpin_frame(ESP);
  if (syscall_num >= sizeof(fun_ptr_arr) / sizeof(fun_ptr_arr[0]))
  {
    terminate_thread(STATUS_FAIL);
  }
  fun_ptr_arr[syscall_num](f);
}

/* Terminates Pintos (this should be seldom used). */
static void
syscall_halt(struct intr_frame *f UNUSED)
{
  shutdown_power_off();
}

/* Terminates the current user program, sending its
   exit status to the kernal. */
static void
syscall_exit(struct intr_frame *f)
{
  check_validation(ARG_0);
  int status = *((int *)(ARG_0));

  struct thread *cur = thread_current();
  printf("%s: exit(%" PRId32 ")\n", cur->name, status);
  /* ensure the file lock has been released */
  if (file_lock.holder == cur)
  {
    lock_release(&file_lock);
  }
  if (cur->parent_status == false && cur->child_status_pointer != NULL)
  {
    *(cur->child_status_pointer) = true;
    *(cur->exit_code) = status;
    sema_up(cur->wait_sema);
  }
  unpin_frame(ARG_0);
  thread_exit();
  NOT_REACHED();
}

/* Runs the executable whose name is given in cmd line, passing any given
   arguments, and returns the new process’s program id (pid). */
static void
syscall_exec(struct intr_frame *f)
{
  check_validation_str(ARG_0);
  char *cmd_line = *(char **)(ARG_0);

  pid_t pid = process_execute(cmd_line);
  unpin_frame(ARG_0);
  f->eax = pid;
}

/* Waits for a child process pid and retrieves the child’s exit status. */
static void
syscall_wait(struct intr_frame *f)
{
  check_validation(ARG_0);
  pid_t pid = *((pid_t *)(ARG_0));

  int status = process_wait(pid);
  unpin_frame(ARG_0);
  f->eax = status;
}

/* Creates a new ﬁle called ﬁle initially initial size bytes in size. */
static void
syscall_create(struct intr_frame *f)
{
  check_validation_str(ARG_0);
  check_validation(ARG_1);
  char *file = *(char **)(ARG_0);
  unsigned initial_size = *((unsigned *)(ARG_1));

  lock_acquire(&file_lock);
  bool success = filesys_create(file, initial_size);
  lock_release(&file_lock);
  unpin_frame(ARG_0);
  unpin_frame(ARG_1);
  f->eax = success;
}

/* Deletes the ﬁle called ﬁle. Returns true if successful, false otherwise. */
static void
syscall_remove(struct intr_frame *f)
{
  check_validation_str(ARG_0);
  char *file = *(char **)(ARG_0);

  lock_acquire(&file_lock);
  bool success = filesys_remove(file);
  lock_release(&file_lock);
  unpin_frame(ARG_0);
  f->eax = success;
}

/* Opens the ﬁle called ﬁle. Returns a nonnegative integer handle called a
  “ﬁle descriptor” (fd), or -1 if the ﬁle could not be opened. */
static void
syscall_open(struct intr_frame *f)
{
  check_validation_str(ARG_0);
  char *file = *(char **)(ARG_0);

  lock_acquire(&file_lock);
  struct file *ff = filesys_open(file);
  if (ff == NULL)
  {
    lock_release(&file_lock);
    f->eax = -1;
    return;
  }
  else
  {
    struct File_info *info = malloc(sizeof(struct File_info));
    if (info == NULL)
    {
      lock_release(&file_lock);
      unpin_frame(ARG_0);
      terminate_thread(STATUS_FAIL);
    }
    info->fd = thread_current()->fd;
    info->file = ff;
    hash_insert(&thread_current()->file_table, &info->elem);
  }
  lock_release(&file_lock);
  unpin_frame(ARG_0);
  f->eax = thread_current()->fd++;
}

/* Returns the size, in bytes, of the ﬁle open as fd. */
static void
syscall_filesize(struct intr_frame *f)
{
  check_validation(ARG_0);
  int fd = *((int *)(ARG_0));

  lock_acquire(&file_lock);
  struct File_info *info = get_file_info(fd);
  if (CHECK_NULL_FILE(info->file))
  {
    lock_release(&file_lock);
    terminate_thread(STATUS_FAIL);
  }
  int size = file_length(info->file);
  lock_release(&file_lock);
  unpin_frame(ARG_0);
  f->eax = size;
}

/* Reads size bytes from the ﬁle open as fd into buﬀer. */
static void
syscall_read(struct intr_frame *f)
{
  check_validation(ARG_0);
  check_validation(ARG_1);
  check_validation(ARG_2);
  check_validation_rw(*(void **)(ARG_1), *((unsigned *)(ARG_2)));
  int fd = *((int *)(ARG_0));
  void *buffer = *(void **)(ARG_1);
  unsigned size = *((unsigned *)(ARG_2));

  /* Reads size bytes from the open file fd into buffer */
  // pin_frame_file(buffer, size);
  if (fd == 0)
  {
    /* Standard input reading */
    for (unsigned i = 0; i < size; i++)
    {
      ((char *)buffer)[i] = input_getc(); // It is a char!
    }
    f->eax = size;
    return;
  }
  lock_acquire(&file_lock);
  struct File_info *info = get_file_info(fd);
  if (CHECK_NULL_FILE(info->file))
  {
    lock_release(&file_lock);
    unpin_frame(ARG_0);
    unpin_frame(ARG_1);
    unpin_frame(ARG_2);
    unpin_frame_file(buffer, size);
    terminate_thread(STATUS_FAIL);
  }
  int read_size = file_read(info->file, buffer, size);
  lock_release(&file_lock);
  unpin_frame(ARG_0);
  unpin_frame(ARG_1);
  unpin_frame(ARG_2);
  unpin_frame_file(buffer, size);
  f->eax = read_size;
}

/* Writes size bytes from buﬀer to the open ﬁle fd. */
static void
syscall_write(struct intr_frame *f)
{
  check_validation(ARG_0);
  check_validation(ARG_1);
  check_validation(ARG_2);
  check_validation_rw(*(void **)(ARG_1), *((unsigned *)(ARG_2)));
  int fd = *((int *)(ARG_0));
  void *buffer = *(void **)(ARG_1);
  unsigned size = *((unsigned *)(ARG_2));

  /* Writes size bytes from buffer to the open file fd */
  // pin_frame_file(buffer, size);
  if (fd == 1)
  {
    /* Standard output writing */
    putbuf(buffer, size);
    f->eax = size;
    return;
  }
  lock_acquire(&file_lock);
  struct File_info *info = get_file_info(fd);
  if (CHECK_NULL_FILE(info->file))
  {
    lock_release(&file_lock);
    unpin_frame(ARG_0);
    unpin_frame(ARG_1);
    unpin_frame(ARG_2);
    unpin_frame_file(buffer, size);
    terminate_thread(STATUS_FAIL);
  }
  int write_size = file_write(info->file, buffer, size);
  lock_release(&file_lock);
  unpin_frame(ARG_0);
  unpin_frame(ARG_1);
  unpin_frame(ARG_2);
  unpin_frame_file(buffer, size);
  f->eax = write_size;
}

/* Changes the next byte to be read or written in open ﬁle fd to position,
   expressed in bytes from the beginning of the ﬁle. */
static void
syscall_seek(struct intr_frame *f)
{
  check_validation(ARG_0);
  check_validation(ARG_1);
  int fd = *((int *)(ARG_0));
  unsigned position = *((unsigned *)(ARG_1));

  lock_acquire(&file_lock);
  struct File_info *info = get_file_info(fd);
  if (info)
  {
    file_seek(info->file, position);
  }
  lock_release(&file_lock);
  unpin_frame(ARG_0);
  unpin_frame(ARG_1);
}

/* Returns the position of the next byte to be read or written in open ﬁle fd,
   expressed in bytes from the beginning of the ﬁle. */
static void
syscall_tell(struct intr_frame *f)
{
  check_validation(ARG_0);
  int fd = *((int *)(ARG_0));

  lock_acquire(&file_lock);
  struct File_info *info = get_file_info(fd);
  if (info)
  {
    file_tell(info->file);
  }
  lock_release(&file_lock);
  unpin_frame(ARG_0);
}

/* Closes ﬁle descriptor fd. Exiting or terminating a process implicitly closes
   all its open ﬁle descriptors, as if by calling this function for each one. */
static void
syscall_close(struct intr_frame *f)
{
  check_validation(ARG_0);
  int fd = *((int *)(ARG_0));

  lock_acquire(&file_lock);
  struct File_info *info = get_file_info(fd);
  if (info)
  {
    file_close(info->file);
    hash_delete(&thread_current()->file_table, &info->elem);
    free(info);
  }
  lock_release(&file_lock);
  unpin_frame(ARG_0);
}

static void syscall_mmap(struct intr_frame *f)
{
  check_validation(ARG_0);
  check_validation(ARG_1);
  int fd = *(int *)(ARG_0);
  uint32_t address = *(uint32_t *)(ARG_1);
  lock_acquire(&file_lock);
  struct File_info *find = get_file_info(fd);
  if (find == NULL)
  {
    f->eax = -1;
    return;
  }

  struct file *file = file_reopen(find->file);
  lock_release(&file_lock);
  struct mmap_elem *adding = malloc(sizeof(struct mmap_elem));
  if (is_stack_address((void *)address, f->esp) || !load_mmap(file, address, adding))
  {
    free(adding);
    f->eax = -1;
    unpin_frame(ARG_0);
    unpin_frame(ARG_1);
    return;
  }

  f->eax = thread_current()->map_int;
  adding->file = file;
  adding->mapid = thread_current()->map_int++;
  adding->page_address = address;
  hash_insert(&thread_current()->mmap_hash, &adding->elem);
  unpin_frame(ARG_0);
  unpin_frame(ARG_1);
}

static void syscall_unmmap(struct intr_frame *f)
{
  check_validation(ARG_0);
  struct mmap_elem temp;
  temp.mapid = *(int *)(ARG_0);
  struct hash_elem *find = hash_find(&thread_current()->mmap_hash, &temp.elem);
  if (find == NULL)
  {
    unpin_frame(ARG_0);
    PANIC("mapid not found");
  }
  hash_delete(&thread_current()->mmap_hash, find);
  lock_acquire(&page_lock);
  munmapHelper(find, NULL);
  lock_release(&page_lock);
  unpin_frame(ARG_0);
}

/* get file info from fd */
static struct File_info *
get_file_info(int fd)
{
  struct File_info key;
  key.fd = fd;
  struct hash_elem *e = hash_find(&thread_current()->file_table, &key.elem);
  if (e != NULL)
  {
    return GET_FILE(e);
  }
  /* If no file_info is found, return NULL */
  return NULL;
}

/* Function used for checking validation for the user virtual address is valid
   and check kernel virtual address from the specific address given. If
   the address given is invalid, call syscall_exit to terminate the process. */
static void check_validation(void *vaddr)
{
  if (vaddr == NULL || !is_user_vaddr(vaddr))
  {
    terminate_thread(STATUS_FAIL);
  }
  else
  {
    page_elem page_elem = page_lookup((uint32_t)pg_round_down(vaddr));
    if (page_elem == NULL)
    {
      terminate_thread(STATUS_FAIL);
    }
    pin_frame(vaddr);
  }
}

/* special case for string that need to check whether content is null */
static void check_validation_str(char **vaddr)
{
  if (*vaddr == NULL)
  {
    terminate_thread(STATUS_FAIL);
  }
  check_validation(vaddr);
}

/* Function used for making sure that the buffer stored the file is valid by
   using for loop.  */
static void check_validation_rw(void *buffer, unsigned size)
{
  /* address of the start of buffer */
  uint32_t local = (uint32_t)pg_round_down(buffer);
  unsigned buffer_length = (uint32_t)buffer + size;
  for (; local < buffer_length; local += PGSIZE)
  {
    if (local < USER_BOTTOM || !is_user_vaddr((const void *)local))
    {
      terminate_thread(STATUS_FAIL);
    }
    check_validation((void *)local);
  }
}

/* unmap all the file in frame */
static void unpin_frame_file(void *uaddr, int size)
{
  lock_acquire(&page_lock);

  uint32_t local = (uint32_t)pg_round_down(uaddr);
  uint32_t buffer_length = (uint32_t)uaddr + size;
  for (; local < buffer_length; local += PGSIZE)
  {
    if (!page_set_pin((uint32_t)local, false))
    {
      lock_release(&page_lock);
      terminate_thread(STATUS_FAIL);
    }
  }

  lock_release(&page_lock);
}

/* set pin and move item into frame if not in frame */
static void pin_frame(void *uaddr)
{
  lock_acquire(&page_lock);
  /* not need to exists if failed */
  if (page_lookup((uint32_t)pg_round_down(uaddr)) == NULL)
  {
    lock_release(&page_lock);
    return;
  }
  page_set_pin((uint32_t)pg_round_down(uaddr), true);
  /* load into frame, doing same thing as page fault*/
  page_elem page = page_lookup((uint32_t)pg_round_down(uaddr));
  if (pagedir_get_page(thread_current()->pagedir, (void *)page->page_address) == NULL)
  {
    if (page->page_status == IN_SWAP)
    {
      void *kpage = swap_back_page(page->page_address);
      if (!install_page((void *)page->page_address, (void *)kpage, page->writable))
      {
        PANIC("install page failed\n");
      }
      pagedir_set_dirty(thread_current()->pagedir, (void *)page->page_address, page->dirty);
    }
    else if (page->page_status == IN_FILE || page->page_status == IS_MMAP)
    {
      load_page(page->lazy_file, page);
    }
    else
    {
      PANIC("pin_frame: page status is wrong\n");
    }
  }
  lock_release(&page_lock);
}

/* unpin the single page */
static void unpin_frame(void *uaddr)
{
  lock_acquire(&page_lock);
  /* not need to exists if failed */
  if (page_lookup((uint32_t)pg_round_down(uaddr)) == NULL)
  {
    lock_release(&page_lock);
    return;
  }
  page_set_pin((uint32_t)pg_round_down(uaddr), false);
  lock_release(&page_lock);
}

/* doing similar thing as syscall exit but receive status as a argument */
void terminate_thread(int status)
{
  struct thread *cur = thread_current();
  printf("%s: exit(%" PRId32 ")\n", cur->name, status);
  /* ensure the file lock has been released */
  if (file_lock.holder == cur)
  {
    lock_release(&file_lock);
  }
  if (cur->parent_status == false && cur->child_status_pointer != NULL)
  {
    *(cur->child_status_pointer) = true;
    *(cur->exit_code) = status;
    sema_up(cur->wait_sema);
  }
  thread_exit();
  NOT_REACHED();
}

static bool
load_mmap(struct file *file, uint32_t upage, struct mmap_elem *mmap_elem)
{
  lock_acquire(&file_lock);
  uint32_t length = file_length(file);
  lock_release(&file_lock);
  if (length == 0 || pg_ofs((void *)upage) != 0 || (void *)upage == NULL)
  {
    return false;
  }
  uint32_t read_bytes = length;
  uint32_t zero_bytes = (PGSIZE - (length % PGSIZE)) % PGSIZE;
  uint32_t oldUpage = upage;
  off_t ofs = 0;
  int page_num = 0;
  while (read_bytes > 0)
  {
    size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;

    lock_acquire(&page_lock);
    if (page_lookup(oldUpage) != NULL)
    {
      lock_release(&page_lock);
      return false;
    }
    lock_release(&page_lock);

    ofs += page_read_bytes;
    read_bytes -= page_read_bytes;
    oldUpage += PGSIZE;
    page_num++;
  }
  mmap_elem->page_num = page_num;
  for (int i = 0; i < page_num; i++)
  {
    lock_acquire(&page_lock);
    struct page_elem *page = page_table_adding(upage + i * PGSIZE, (uint32_t)NULL, IS_MMAP);
    lock_release(&page_lock);
    page->lazy_file = malloc(sizeof(struct lazy_file));
    page->lazy_file->file = file;
    page->lazy_file->offset = i * PGSIZE;
    if (i == page_num - 1)
    {
      page->lazy_file->read_bytes = PGSIZE - zero_bytes;
      page->lazy_file->zero_bytes = zero_bytes;
    }
    else
    {
      page->lazy_file->read_bytes = PGSIZE;
      page->lazy_file->zero_bytes = 0;
    }
    page->writable = true;
    page->swapped_id = -1;
  }

  return true;
}

/* a helper function for munmap */
void munmapHelper(struct hash_elem *found_elem, void *aux UNUSED)
{
  struct mmap_elem *found = hash_entry(found_elem, struct mmap_elem, elem);
  int n = found->page_num;
  for (int i = 0; i < n; i++)
  {
    uint32_t page = found->page_address + i * PGSIZE;
    if (pagedir_get_page(thread_current()->pagedir, (void *)page) != NULL)
    {
      // only write back if it is dirty
      if (pagedir_is_dirty(thread_current()->pagedir, (void *)page))
      {
        ASSERT((void *)page_lookup(page)->kernel_address != NULL);
        lock_acquire(&file_lock);
        file_write_at(found->file, (void *)page_lookup(page)->kernel_address, PGSIZE, page_lookup(page)->lazy_file->offset);
        lock_release(&file_lock);
      }
    }
    pagedir_clear_page(thread_current()->pagedir, (void *)page);
    page_clear(page);
  }
  lock_acquire(&file_lock);
  file_close(found->file);
  lock_release(&file_lock);
  free(found);
}