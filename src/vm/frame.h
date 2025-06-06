#ifndef FRAME_H
#define FRAME_H

#include "lib/kernel/hash.h"
#include "lib/kernel/list.h"
#include "lib/kernel/bitmap.h"
#include "pageTable.h"

#define get_frame_hash_elem(ELEM) hash_entry(ELEM, struct frame_elem, hash_e)
#define get_frame_list_elem(ELEM) list_entry(ELEM, struct frame_elem, list_e)
#define get_pd(ELEM) get_frame_list_elem(ELEM)->ppage->pd
struct frame_elem
{
    struct list_elem list_e;
    struct hash_elem hash_e;
    uint32_t frame_addr;
    struct page_elem *ppage;
};

void frame_init(void);
void frame_add(uint32_t, struct page_elem *);
void frame_free(uint32_t);
hash_action_func frame_free_action;
void frame_swap(void);

#endif