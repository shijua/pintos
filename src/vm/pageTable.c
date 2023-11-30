#include "vm/pageTable.h"
#include "vm/frame.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "devices/swap.h"
#include "threads/palloc.h"
#include <debug.h>

unsigned page_hash_func (const struct hash_elem *element, void *aux) {
  return getPageElem(element)->page_address;
}

bool page_less_func (const struct hash_elem *a, const struct hash_elem *b, void *aux) {
  return getPageElem(a)->page_address <  getPageElem(b)->page_address ;
}

bool
pageTableAdding (const uint32_t page_address, const uint32_t kernel_address, enum status status) {
    // ASSERT (pageLookUp(page_address) == NULL); // make sure this page is not in the supplemental page table
    page_elem adding = malloc(sizeof(struct page_elem));
    if(adding == NULL) {
        PANIC("malloc failed");
        return false;
    }
    adding->page_address = page_address;
    adding->pd           = thread_current()->pagedir;
    adding->kernel_address = kernel_address;
    adding->page_status = status;
    adding->swapped_id = -1;
    hash_insert(&thread_current()->supplemental_page_table, &adding->elem);
    return true;
}

void
pageFree (const uint32_t page_address) {
    struct page_elem temp;
    temp.page_address = page_address;
    struct hash_elem *find = hash_find(&thread_current()->supplemental_page_table, &temp.elem);
    if(find == NULL){
        PANIC("page not existing");
    }
    page_elem removing = getPageElem(find);
    // TODO this page may not be in the frame
    // either free the item in frame or in swap
    frame_free(removing->kernel_address);
    hash_delete(&thread_current()->supplemental_page_table, &removing->elem);
    free(removing);
}

void
swapBackPage (const uint32_t page_address) {
    struct page_elem temp;
    temp.page_address = page_address;
    struct hash_elem *find = hash_find(&thread_current()->supplemental_page_table, &temp.elem);
    if(find == NULL){
        PANIC("page not existing");
    }
    page_elem elem = getPageElem(find);
    ASSERT(elem->page_status == IN_SWAP);
    uint32_t kernel_address = palloc_get_page(PAL_USER);
    swap_in(kernel_address, elem->swapped_id);
    elem->page_status = IN_FRAME;
}


page_elem
pageLookUp (const uint32_t page_address) {
    struct page_elem temp;
    temp.page_address = page_address;
    struct hash_elem *find = hash_find(&thread_current()->supplemental_page_table, &temp.elem);
    if(find == NULL){
      return NULL; // which means this page is not in the supplemental page table, so it is not a valid page
    }
    return getPageElem(find);
}

