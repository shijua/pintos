#ifndef PAGE_TABLE_H
#define PAGE_TABLE_H

#include "lib/kernel/hash.h"
#include "lib/kernel/list.h"
#include "lib/kernel/bitmap.h"
#include <stdint.h>

#define getPageElem(ELEM) hash_entry (ELEM, struct page_elem, elem)

typedef struct page_elem {
   struct hash_elem elem;
   uint32_t *pd;
   uint32_t page_address;
   uint32_t physical_address;
   bool swaped;
   size_t swapped_id;
} *page_elem;

#endif