#ifndef PAGE_TABLE_H
#define PAGE_TABLE_H

#include "lib/kernel/hash.h"
#include "lib/kernel/list.h"
#include "lib/kernel/bitmap.h"
#include "filesys/off_t.h"
#include <stdint.h>
#include "threads/synch.h"

#define getPageElem(ELEM) hash_entry (ELEM, struct page_elem, elem)

unsigned page_hash_func (const struct hash_elem *element, void *aux);
bool page_less_func (const struct hash_elem *a, const struct hash_elem *b, void *aux);

enum page_status {
   IN_SWAP,
   IN_FRAME,
   IN_FILE,
   IS_MMAP
};

struct lazy_file {
   struct file *file;
   off_t offset;
   size_t read_bytes;
   size_t zero_bytes;
   uint32_t kernel_address;
};


typedef struct page_elem {
   struct hash_elem elem;
   uint32_t *pd;
   uint32_t page_address;
   enum page_status page_status;
   struct lazy_file *lazy_file;
   uint32_t kernel_address;
   size_t swapped_id;
   bool writable;
   bool dirty;
   bool is_pin;
} *page_elem;

page_elem pageTableAdding (const uint32_t, const uint32_t, enum page_status);
void page_clear (const uint32_t);
hash_action_func page_free_action;
void *swapBackPage (const uint32_t page_address);
page_elem pageLookUp (const uint32_t page_address);
bool page_set_pin (uint32_t page_address, bool pin);

#endif