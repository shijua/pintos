#include "filesys/file.h"
#include "lib/kernel/hash.h"
#include "threads/vaddr.h"
#include "vm/pageTable.h"

#define get_zero_byte(READ_BYTE) (PGSIZE - (READ_BYTE % PGSIZE)) % PGSIZE

typedef struct executable_file_elem
{
    struct file *file;
    struct hash_elem elem;
    int loaded;
    struct list lazy_file_list;
} *executable_file_elem;

void init_exe_hash(void);
struct lazy_file *
exe_set_j(struct executable_file_elem *elem,
          int j,
          struct file *file,
          off_t offset,
          size_t read_bytes,
          size_t zero_bytes,
          struct page_elem *ppage);
struct lazy_file *
exe_get_j(struct executable_file_elem *elem,
          int j);
void exe_remove(struct executable_file_elem *);
void exe_add(struct executable_file_elem *);
executable_file_elem exe_get_create(struct file *);