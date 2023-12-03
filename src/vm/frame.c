#include "threads/thread.h"
#include "threads/synch.h"
#include "frame.h"
#include "devices/swap.h"
#include "threads/malloc.h"
#include "userprog/pagedir.h"
static struct list frame_list;
static struct hash frame_hash;
static struct lock frame_lock;
static struct list_elem *frame_pointer;
static hash_hash_func frame_hash_func;
static hash_less_func frame_less_func;

void
frame_init(){
    list_init(&frame_list);
    hash_init(&frame_hash, frame_hash_func, frame_less_func, NULL);
    lock_init(&frame_lock);
    frame_pointer = list_tail(&frame_list);
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
  if(adding == NULL) {
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

  pagedir_set_accessed(adding->ppage->pd, adding->ppage->page_address, true);
  lock_release(&frame_lock);
}

void frame_set_page (uint32_t kernel_addr, struct page_elem *page) {
  lock_acquire(&frame_lock);
  struct frame_elem temp;
  temp.frame_addr = kernel_addr;
  struct hash_elem *hashElem = hash_find(&frame_hash, &temp.hash_e);
  if(hashElem == NULL) {
    PANIC("frame_set_page: frame not found");
  }
  struct frame_elem *frame_elem = getFrameHashElem(hashElem);
  pagedir_set_accessed(frame_elem->ppage->pd, frame_elem->ppage->page_address, true);
  frame_elem->ppage = page;
  lock_release(&frame_lock);
}


// change to void
bool
frame_free (uint32_t kernel_addr){
  lock_acquire(&frame_lock);
  struct frame_elem temp;
  temp.frame_addr = kernel_addr;
  struct hash_elem *hashElem = hash_find(&frame_hash, &temp.hash_e);
  if(hashElem == NULL) {
    lock_release(&frame_lock);
    PANIC("frame_free: frame not found");
    return false;
  }
  
  struct frame_elem *removing = getFrameHashElem(hashElem);
  list_remove(&removing->list_e);
  if(frame_pointer == &removing->list_e){
    frame_index_loop();
  }
  hash_delete(&frame_hash, hashElem);
  free(removing);
  lock_release(&frame_lock);
  return true;
}

void
frame_free_action (struct hash_elem *element, void *aux){
  lock_acquire (&frame_lock);
  struct frame_elem *removing = getFrameHashElem(element);
  list_remove(&removing->list_e);
  if(frame_pointer == &removing->list_e){
    frame_index_loop();
  }
  free(removing);
  lock_release (&frame_lock);
}


uint32_t
frame_swap () {
  lock_acquire(&frame_lock);
  if(frame_pointer == NULL){
    PANIC("frame_swap: frame_pointer is NULL");
    lock_release(&frame_lock);
    return 0;
  }
  while(pagedir_is_accessed(getPd(frame_pointer), getFrameListElem(frame_pointer)->ppage->page_address) == true){
    pagedir_set_accessed(getPd(frame_pointer), getFrameListElem(frame_pointer)->ppage->page_address, false);
    frame_index_loop();
  }
  struct frame_elem *frame_elem = getFrameListElem(frame_pointer);
  frame_elem->ppage->page_status = IN_SWAP;
  frame_elem->ppage->swapped_id = swap_out(&frame_elem->frame_addr);
  frame_elem->ppage->writable = pagedir_is_writable(getPd(frame_pointer), getFrameListElem(frame_pointer)->ppage->page_address);
  pagedir_clear_page (frame_elem->ppage->pd, frame_elem->ppage->page_address);
  frame_index_loop();
  lock_release(&frame_lock);
  return frame_elem->frame_addr;
}



