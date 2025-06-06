#include "userprog/exception.h"
#include <inttypes.h>
#include <stdio.h>
#include "userprog/gdt.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "userprog/syscall.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "threads/palloc.h"
#include <string.h>
#include "vm/frame.h"
#include "vm/pageTable.h"
#include "threads/thread.h"
#include "filesys/file.h"
#include "userprog/process.h"
#include "threads/malloc.h"

static void grow_stack(void *fault_addr);

// function used to check if pointer mapped to a unmapped memory
static bool
is_valid_ptr(const void *user_ptr)
{
  struct thread *curr = thread_current();
  if (user_ptr != NULL && is_user_vaddr(user_ptr))
  {
    return pagedir_get_page(curr->pagedir, user_ptr) != NULL;
  }
  return false;
}

/* Number of page faults processed. */
static long long page_fault_cnt;

static void kill(struct intr_frame *);
static void page_fault(struct intr_frame *);

/* Registers handlers for interrupts that can be caused by user
   programs.

   In a real Unix-like OS, most of these interrupts would be
   passed along to the user process in the form of signals, as
   described in [SV-386] 3-24 and 3-25, but we don't implement
   signals.  Instead, we'll make them simply kill the user
   process.

   Page faults are an exception.  Here they are treated the same
   way as other exceptions, but this will need to change to
   implement virtual memory.

   Refer to [IA32-v3a] section 5.15 "Exception and Interrupt
   Reference" for a description of each of these exceptions. */
void exception_init(void)
{
  /* These exceptions can be raised explicitly by a user program,
     e.g. via the INT, INT3, INTO, and BOUND instructions.  Thus,
     we set DPL==3, meaning that user programs are allowed to
     invoke them via these instructions. */
  intr_register_int(3, 3, INTR_ON, kill, "#BP Breakpoint Exception");
  intr_register_int(4, 3, INTR_ON, kill, "#OF Overflow Exception");
  intr_register_int(5, 3, INTR_ON, kill, "#BR BOUND Range Exceeded Exception");

  /* These exceptions have DPL==0, preventing user processes from
     invoking them via the INT instruction.  They can still be
     caused indirectly, e.g. #DE can be caused by dividing by
     0.  */
  intr_register_int(0, 0, INTR_ON, kill, "#DE Divide Error");
  intr_register_int(1, 0, INTR_ON, kill, "#DB Debug Exception");
  intr_register_int(6, 0, INTR_ON, kill, "#UD Invalid Opcode Exception");
  intr_register_int(7, 0, INTR_ON, kill, "#NM Device Not Available Exception");
  intr_register_int(11, 0, INTR_ON, kill, "#NP Segment Not Present");
  intr_register_int(12, 0, INTR_ON, kill, "#SS Stack Fault Exception");
  intr_register_int(13, 0, INTR_ON, kill, "#GP General Protection Exception");
  intr_register_int(16, 0, INTR_ON, kill, "#MF x87 FPU Floating-Point Error");
  intr_register_int(19, 0, INTR_ON, kill, "#XF SIMD Floating-Point Exception");

  /* Most exceptions can be handled with interrupts turned on.
     We need to disable interrupts for page faults because the
     fault address is stored in CR2 and needs to be preserved. */
  intr_register_int(14, 0, INTR_OFF, page_fault, "#PF Page-Fault Exception");
}

/* Prints exception statistics. */
void exception_print_stats(void)
{
  printf("Exception: %lld page faults\n", page_fault_cnt);
}

/* Handler for an exception (probably) caused by a user process. */
static void
kill(struct intr_frame *f)
{
  /* This interrupt is one (probably) caused by a user process.
     For example, the process might have tried to access unmapped
     virtual memory (a page fault).  For now, we simply kill the
     user process.  Later, we'll want to handle page faults in
     the kernel.  Real Unix-like operating systems pass most
     exceptions back to the process via signals, but we don't
     implement them. */

  /* The interrupt frame's code segment value tells us where the
     exception originated. */
  switch (f->cs)
  {
  case SEL_UCSEG:
    /* User's code segment, so it's a user exception, as we
       expected.  Kill the user process.  */
    printf("%s: dying due to interrupt %#04x (%s).\n",
           thread_name(), f->vec_no, intr_name(f->vec_no));
    intr_dump_frame(f);
    thread_exit();

  case SEL_KCSEG:
    /* Kernel's code segment, which indicates a kernel bug.
       Kernel code shouldn't throw exceptions.  (Page faults
       may cause kernel exceptions--but they shouldn't arrive
       here.)  Panic the kernel to make the point.  */
    intr_dump_frame(f);
    PANIC("Kernel bug - unexpected interrupt in kernel");

  default:
    /* Some other code segment?
       Shouldn't happen.  Panic the kernel. */
    printf("Interrupt %#04x (%s) in unknown segment %04x\n",
           f->vec_no, intr_name(f->vec_no), f->cs);
    PANIC("Kernel bug - this shouldn't be possible!");
  }
}

/* Page fault handler.  This is a skeleton that must be filled in
   to implement virtual memory.  Some solutions to task 2 may
   also require modifying this code.

   At entry, the address that faulted is in CR2 (Control Register
   2) and information about the fault, formatted as described in
   the PF_* macros in exception.h, is in F's error_code member.  The
   example code here shows how to parse that information.  You
   can find more information about both of these in the
   description of "Interrupt 14--Page Fault Exception (#PF)" in
   [IA32-v3a] section 5.15 "Exception and Interrupt Reference". */
static void
page_fault(struct intr_frame *f)
{
  bool not_present; /* True: not-present page, false: writing r/o page. */
  bool write;       /* True: access was write, false: access was read. */
  bool user;        /* True: access by user, false: access by kernel. */
  void *fault_addr; /* Fault address. */

  /* Obtain faulting address, the virtual address that was
     accessed to cause the fault.  It may point to code or to
     data.  It is not necessarily the address of the instruction
     that caused the fault (that's f->eip).
     See [IA32-v2a] "MOV--Move to/from Control Registers" and
     [IA32-v3a] 5.15 "Interrupt 14--Page Fault Exception
     (#PF)". */
  asm("movl %%cr2, %0" : "=r"(fault_addr));

  /* Turn interrupts back on (they were only off so that we could
     be assured of reading CR2 before it changed). */
  intr_enable();
  /* Determine cause. */
  not_present = (f->error_code & PF_P) == 0;
  write = (f->error_code & PF_W) != 0;
  user = (f->error_code & PF_U) != 0;
  lock_acquire(&page_lock);

  struct page_elem *page = page_lookup((uint32_t)pg_round_down(fault_addr));

  /* check if it is a stack access */
  void *esp = f->esp;
  if (page == NULL && is_stack_address(fault_addr, esp))
  {
    /* grow the stack */
    grow_stack(pg_round_down(fault_addr));
    lock_release(&page_lock);
    return;
  }

  if (page != NULL)
  { // it is a fake page fault
    switch (page->page_status)
    {
    case IN_FRAME:
      /* error if it is a frame but go to page fault */
      lock_release(&page_lock);
      terminate_thread(STATUS_FAIL);
      break;
    case IN_SWAP:
      /* swap the page back into frame */
      void *kpage = swap_back_page(page->page_address);
      if (!install_page((void *)page->page_address, kpage, page->writable))
      {
        PANIC("install page failed\n");
      }
      pagedir_set_dirty(thread_current()->pagedir, (void *)page->page_address, page->dirty);
      lock_release(&page_lock);
      break;
    default: // for both mmap and file
      /* if the lock is not released when coming to interrupt */
      load_page(page->lazy_file, page);
      lock_release(&page_lock);
      break;
    }
    return;
  }
  lock_release(&page_lock);
  /* Count page faults. */
  page_fault_cnt++;

  /* check if the memory is unmapped */
  if (!is_valid_ptr(fault_addr))
  {
    terminate_thread(STATUS_FAIL);
  }
  /* To implement virtual memory, delete the rest of the function
     body, and replace it with code that brings in the page to
     which fault_addr refers. */
  printf("Page fault at %p: %s error %s page in %s context.\n",
         fault_addr,
         not_present ? "not present" : "rights violation",
         write ? "writing" : "reading",
         user ? "user" : "kernel");
  kill(f);
}
/* check it is a stack access, we need to grow the stack */
bool is_stack_address(void *fault_addr, void *esp)
{
  // if it is outside stack range return false
  if (fault_addr >= PHYS_BASE || fault_addr < PHYS_BASE - STACK_MAX)
  {
    return false;
  }
  // several condition that will make it a stack access
  if (esp - PUSH_A_SIZE == fault_addr || esp - PUSH_SIZE == fault_addr || esp <= fault_addr)
  {
    return true;
  }
  return false;
}

/* grow the stack */
static void
grow_stack(void *round_addr)
{
  struct thread *curr = thread_current();
  if (curr->stack_size + PGSIZE >= STACK_MAX)
  {
    lock_release(&page_lock);
    terminate_thread(STATUS_FAIL);
  }
  /* allocate a new page */
  void *kpage = palloc_get_page(PAL_USER);
  /* add the page to the supplemental page table */
  page_table_adding((uint32_t)round_addr, (uint32_t)kpage, IN_FRAME);
  /* add the page to the process's address space */
  if (!install_page(round_addr, kpage, true))
  {
    palloc_free_page(kpage);
    PANIC("install page failed");
  }
  /* update the stack size */
  curr->stack_size += PGSIZE;
}

/* load single page */
void load_page(struct lazy_file *Lfile, struct page_elem *page)
{

  /* Check if virtual page already allocated */
  struct thread *t = thread_current();
  void *kpage = pagedir_get_page(t->pagedir, (void *)page->page_address);
  ASSERT(page != NULL);
  if (page->page_status != IS_MMAP)
  {
    page->page_status = IN_FRAME;
  }
  if (kpage == NULL)
  {

    /* Get a new page of memory. */
    kpage = palloc_get_page(PAL_USER);
    ASSERT(kpage != NULL);

    /* Add the page to the process's address space. */
    if (!install_page((void *)page->page_address, kpage, page->writable))
    {
      palloc_free_page(kpage);
      PANIC("install page failed\n");
    }
    page->kernel_address = (uint32_t)kpage;
  }
  else
  {
    /* Check if writable flag for the page should be updated */
    if (page->writable && !pagedir_is_writable(t->pagedir, (void *)page->page_address))
    {
      pagedir_set_writable(t->pagedir, (void *)page->page_address, page->writable);
    }
  }
  /* Load data into the page. */
  lock_acquire(&file_lock);
  if (file_read_at(Lfile->file, kpage, Lfile->read_bytes, Lfile->offset) != (int)Lfile->read_bytes)
  {
    lock_release(&file_lock);
    PANIC("load page failed\n");
  }
  lock_release(&file_lock);
  memset(kpage + Lfile->read_bytes, 0, Lfile->zero_bytes);
}
