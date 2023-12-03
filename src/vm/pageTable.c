#include "vm/pageTable.h"
#include "vm/frame.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "devices/swap.h"
#include "threads/palloc.h"
#include "userprog/pagedir.h"
#include <debug.h>

unsigned page_hash_func (const struct hash_elem *element, void *aux UNUSED) {
  return getPageElem(element)->page_address;
}

bool page_less_func (const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED) {
  return getPageElem(a)->page_address <  getPageElem(b)->page_address ;
}

// TODO: this function change to void
void pageTableAdding (const uint32_t page_address, const uint32_t kernel_address, enum status status) {
    lock_acquire(&page_lock);
    ASSERT (pageLookUp(page_address) == NULL); // make sure this page is not in the supplemental page table
    page_elem adding = malloc(sizeof(struct page_elem));
    if(adding == NULL) {
        PANIC("malloc failed");
    }
    adding->page_address = page_address;
    adding->pd           = thread_current()->pagedir;
    adding->kernel_address = kernel_address;
    adding->page_status = status;
    adding->swapped_id = -1;
    hash_insert(&thread_current()->supplemental_page_table, &adding->elem);
    lock_release(&page_lock);
}

void
page_free_action (struct hash_elem *element, void *aux UNUSED) {
    lock_acquire(&page_lock);
    page_elem removing = getPageElem(element);
    ASSERT (removing != NULL);
    switch (removing->page_status) {
      case IN_FRAME:
          frame_free (removing->kernel_address);
          break;
      case IN_SWAP:
          swap_drop (removing->swapped_id);
          break;
      case IN_FILE:
          free (removing->lazy_file);
          break;
    }
    free(removing);
    lock_release(&page_lock);
}

void *
swapBackPage (const uint32_t page_address) {
    lock_acquire(&page_lock);
    struct page_elem temp;
    temp.page_address = page_address;
    struct hash_elem *find = hash_find(&thread_current()->supplemental_page_table, &temp.elem);
    if(find == NULL) {
        PANIC("page not existing");
    }
    page_elem elem = getPageElem(find);
    ASSERT(elem->page_status == IN_SWAP);
    void* kernel_address = palloc_get_page(PAL_USER);
    swap_in((void *) kernel_address, elem->swapped_id);
    elem->kernel_address = (uint32_t) kernel_address;
    elem->page_status = IN_FRAME;
    lock_release(&page_lock);
    return kernel_address;
}


page_elem
pageLookUp (const uint32_t page_address) {
    bool locked_by_own = false;
    if (!lock_held_by_current_thread(&page_lock)) {
        lock_acquire(&page_lock);
        locked_by_own = true;
    }
    // lock_acquire(&page_lock);
    struct page_elem temp;
    temp.page_address = page_address;
    struct hash_elem *find = hash_find(&thread_current()->supplemental_page_table, &temp.elem);
    if(find == NULL) {
      if (locked_by_own) lock_release(&page_lock);
      return NULL; // which means this page is not in the supplemental page table, so it is not a valid page
    }
    if (locked_by_own) lock_release(&page_lock);
    return getPageElem(find);
}

