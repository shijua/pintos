#include "threads/thread.h"
#include "threads/synch.h"
#include "frame.h"
#include "devices/swap.h"
#include "threads/malloc.h"
#include "userprog/pagedir.h"
#include "threads/palloc.h"
#include "filesys/file.h"
#include "threads/vaddr.h"
#include "stdio.h"
#include "string.h"

static struct lock frame_lock;
static struct list frame_list;
static struct hash frame_hash;
static struct list_elem *frame_pointer;
static hash_hash_func frame_hash_func;
static hash_less_func frame_less_func;

void
frame_init() {
  list_init(&frame_list);
  hash_init(&frame_hash, frame_hash_func, frame_less_func, NULL);
  frame_pointer = list_tail(&frame_list);
  lock_init(&frame_lock);
}

unsigned
frame_hash_func (const struct hash_elem *element, void *aux UNUSED) {
  return getFrameHashElem(element)->frame_addr;
}

bool 
frame_less_func (const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED) {
  return getFrameHashElem(a)->frame_addr <  getFrameHashElem(b)->frame_addr;
}

void
frame_index_loop() {
  if(frame_pointer == list_back(&frame_list)){
    frame_pointer = list_begin(&frame_list);
    return;
  }
  frame_pointer = frame_pointer -> next;
}

void frame_add (uint32_t frame_addr, struct page_elem *page) {
  lock_acquire(&frame_lock);
  struct frame_elem *adding = malloc(sizeof(struct frame_elem));
  if(page == NULL) {
    PANIC("frame_add: malloc failed");
  }
  adding->frame_addr = frame_addr;
  adding->ppage = page;
  hash_insert(&frame_hash, &adding->hash_e);
  list_insert(frame_pointer, &adding->list_e);
  // if the frame was empty before adding
  if(frame_pointer == list_tail(&frame_list)){
    frame_pointer = list_front(&frame_list);
  }

  pagedir_set_accessed(adding->ppage->pd, (void *) adding->ppage->page_address, true);
  lock_release(&frame_lock);
}

void
frame_free (uint32_t kernel_addr) {
  bool locked_by_own = false;
  if (!lock_held_by_current_thread(&frame_lock)) {
      lock_acquire(&frame_lock);
      locked_by_own = true;
  }
  struct frame_elem temp;
  temp.frame_addr = kernel_addr;
  struct hash_elem *hashElem = hash_find(&frame_hash, &temp.hash_e);
  if(hashElem == NULL) {
    PANIC("frame_free: frame not found");
  }
  
  struct frame_elem *removing = getFrameHashElem(hashElem);
  if(frame_pointer == &removing->list_e){
    frame_index_loop();
  }
  list_remove(&removing->list_e);
  hash_delete(&frame_hash, hashElem);
  free(removing);
  if (locked_by_own) {
    lock_release(&frame_lock);
  }
}

void
frame_free_action (struct hash_elem *element, void *aux UNUSED){
  lock_acquire (&frame_lock);
  struct frame_elem *removing = getFrameHashElem(element);
  list_remove(&removing->list_e);
  if(frame_pointer == &removing->list_e){
    frame_index_loop();
  }
  free(removing);
  lock_release (&frame_lock);
}


void
frame_swap () {
  lock_acquire(&frame_lock);
  if(frame_pointer == NULL){
    PANIC("frame_swap: frame_pointer is NULL");
  }
  // search until it is not pinned and not accessed
  struct frame_elem *frame_elem = getFrameListElem(frame_pointer);
  bool locked_by_own = false;
  if (!lock_held_by_current_thread(&page_lock) && frame_elem->ppage != NULL) {
      ASSERT (frame_elem->ppage->kernel_address != NULL);
      lock_acquire(&page_lock);
      locked_by_own = true;
  }
  while(frame_elem->ppage->is_pin ||
   pagedir_is_accessed(getPd(frame_pointer), 
                      (void *) frame_elem->ppage->page_address) == true){
    pagedir_set_accessed(getPd(frame_pointer), (void *) frame_elem->ppage->page_address, false);
    frame_index_loop();
    frame_elem = getFrameListElem(frame_pointer);
  }
  
  if(frame_elem->ppage->page_status == IS_MMAP ){
    if(pagedir_is_dirty(frame_elem->ppage->pd, frame_elem->ppage->page_address)) {
      lock_acquire(&file_lock);
      file_write_at(frame_elem->ppage->lazy_file->file, 
        (void *)frame_elem->frame_addr, PGSIZE, frame_elem->ppage->lazy_file->offset);
      lock_release(&file_lock);
    }
    
  } else if(frame_elem->ppage->page_status == IN_FILE){

  } else{
    frame_elem->ppage->page_status = IN_SWAP;
    frame_elem->ppage->swapped_id = swap_out((void *) frame_elem->frame_addr);
    frame_elem->ppage->writable = pagedir_is_writable(getPd(frame_pointer), (void *) getFrameListElem(frame_pointer)->ppage->page_address);
    frame_elem->ppage->dirty = pagedir_is_dirty(getPd(frame_pointer), (void *) getFrameListElem(frame_pointer)->ppage->page_address);
  }
  frame_elem->ppage->kernel_address = NULL;
  /* page need be reallocate later as kernel may request and will not add to the frame */
  palloc_free_page((void *) frame_elem->frame_addr);
  pagedir_clear_page (frame_elem->ppage->pd, (void *) frame_elem->ppage->page_address);
  if (locked_by_own) {
    lock_release(&page_lock);
  }
  frame_index_loop();
  frame_free(frame_elem->frame_addr);
  lock_release(&frame_lock);
}



