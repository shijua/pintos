#include "vm/executableFileList.h"
#include "threads/synch.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "vm/pageTable.h"


static struct hash exe_file_hash;
static struct lock exe_file_lock;
static hash_hash_func exe_hash_func;
static hash_less_func exe_less_func;
void
init_exe_hash() 
{
    hash_init(&exe_file_hash, exe_hash_func, exe_less_func, NULL);
    lock_init(&exe_file_lock);
}

unsigned
exe_hash_func (const struct hash_elem *element, void *aux UNUSED) {
  return file_hash(hash_entry(element, struct executable_file_elem, elem)->file);
}

bool 
exe_less_func (const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED) {
  return file_hash(hash_entry(a, struct executable_file_elem, elem)->file)
    <  file_hash(hash_entry(b, struct executable_file_elem, elem)->file);
}

void
exe_remove (struct executable_file_elem* elem) {
    lock_acquire(&exe_file_lock);
    elem->loaded--;
    if(elem->loaded == 0) {
        hash_delete(&exe_file_hash, &elem->elem);
        free(elem);
    }
    lock_release(&exe_file_lock);
}

void
exe_add (struct executable_file_elem* elem) {
    lock_acquire(&exe_file_lock);
    elem->loaded++;
    lock_release(&exe_file_lock);
}

executable_file_elem
exe_get_create (struct file* file, uint32_t size) {
    lock_acquire(&exe_file_lock);
    struct executable_file_elem temp;
    temp.file = file;
    struct hash_elem *find = hash_find(&exe_file_hash, &temp.elem);
    if(find == NULL) {
        struct executable_file_elem *adding = malloc(sizeof(struct executable_file_elem));
        adding -> file = file;
        adding -> lazy_file_list = calloc((size / PGSIZE), sizeof(struct lazy_file *));
        hash_insert(&exe_file_hash, &adding->elem);
        lock_release(&exe_file_lock);
        return adding;
    }
    lock_release(&exe_file_lock);
    return hash_entry(find, struct executable_file_elem, elem);
}
// TODO free