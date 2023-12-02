#ifndef PAGE_TABLE_H
#define PAGE_TABLE_H

#include "lib/kernel/hash.h"
#include "lib/kernel/list.h"
#include "lib/kernel/bitmap.h"
#include "filesys/off_t.h"
#include <stdint.h>

#define getPageElem(ELEM) hash_entry (ELEM, struct page_elem, elem)

unsigned page_hash_func (const struct hash_elem *element, void *aux);
bool page_less_func (const struct hash_elem *a, const struct hash_elem *b, void *aux);

enum status {
   IN_SWAP,
   IN_FRAME,
   IN_FILE
};

struct lazy_file {
   struct file *file;
   off_t offset;
   size_t read_bytes;
   size_t zero_bytes;
   bool writable;
};


typedef struct page_elem {
   struct hash_elem elem;
   uint32_t *pd;
   uint32_t page_address;
   enum status page_status;
   // union info {
   struct lazy_file *lazy_file;
   uint32_t kernel_address;
   size_t swapped_id;
   // }
} *page_elem;

void pageTableAdding (const uint32_t page_address, const uint32_t kernel_address, enum status status);
hash_action_func page_free_action;
void swapBackPage (const uint32_t page_address);
page_elem pageLookUp (const uint32_t page_address);

#endif