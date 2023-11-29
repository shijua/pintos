
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
frame_hash_func (const struct hash_elem *element, void *aux) {
  return getFrameHashElem(element)->frame_addr;
}

bool 
frame_less_func (const struct hash_elem *a, const struct hash_elem *b, void *aux) {
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

bool
frame_add (uint32_t frame_addr, struct page_elem *page) {
  //// sync
  struct frame_elem *adding = malloc(sizeof(struct frame_elem));
  if(adding == NULL) {
    return false;
  }
  adding->frame_addr = frame_addr;
  adding->ppage = page;
  pagedir_set_accessed(getPd(frame_pointer), getFrameListElem(frame_pointer)->ppage->page_address, true);
  hash_insert(&frame_hash, &adding->hash_e);
  list_insert(frame_pointer, &adding->list_e);
  // if the frame was empty before adding
  if(frame_pointer == list_tail(&frame_list)){
    frame_pointer = list_front(&frame_list);
  }
  return true;
}

bool
frame_free (uint32_t kernel_addr){
  //////////// synchronize!!
  struct frame_elem temp;
  temp.frame_addr = kernel_addr;
  struct hash_elem *hashElem = hash_find(&frame_hash, &temp.hash_e);
  if(hashElem == NULL) {
    ////////////////////// need to edit to PANIC
    return false;
  }
  
  struct frame_elem *removing = getFrameHashElem(hashElem);
  list_remove(&removing->list_e);
  if(frame_pointer == &removing->list_e){
    frame_index_loop();
  }
  hash_delete(&frame_hash, hashElem);
  free(removing);
  return true;
}

uint32_t
frame_swap () {
  //////////// synchronize!!
  if(frame_pointer == NULL){
    /////// need panic
    return 0;
  }
  while(pagedir_is_accessed(getPd(frame_pointer), getFrameListElem(frame_pointer)->ppage->page_address) == true){
    pagedir_set_accessed(getPd(frame_pointer), getFrameListElem(frame_pointer)->ppage->page_address, false);
    frame_index_loop();
  }
  struct frame_elem *frame_elem = getFrameListElem(frame_pointer);
  frame_elem->ppage->page_status = IN_SWAP;
  frame_elem->ppage->swapped_id = swap_out(&frame_elem->frame_addr);
  return frame_elem->frame_addr;
}



