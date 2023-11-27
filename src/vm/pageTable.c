#include "vm/pageTable.h"
#include "vm/frame.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "devices/swap.h"
#include "threads/palloc.h"
static hash_hash_func page_hash_fun;
static hash_less_func page_less_fun;

unsigned page_hash_fun (const struct hash_elem *element, void *aux) {
  return getPageElem(element)->page_address;
}

bool page_less_func (const struct hash_elem *a, const struct hash_elem *b, void *aux) {
  return getPageElem(a)->page_address <  getPageElem(b)->page_address ;
}

bool
pageTableAdding (const uint32_t kernel_address, const uint32_t page_address) {
    page_elem adding = malloc(sizeof(struct page_elem));
    if(adding == NULL || !frame_add(kernel_address, adding)) {
        // Panic
        return false;
    }
    adding->page_address = page_address;
    adding->pd           = thread_current()->pagedir;
    adding->physical_address = kernel_address;
    adding->swaped = false;
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
    frame_free(removing->physical_address);
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
    ASSERT(elem->swaped == true);
    uint32_t physical_address = palloc_get_page(PAL_USER);
    swap_in(physical_address, elem->swapped_id);
    elem->swaped = false;
}

