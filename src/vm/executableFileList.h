#include "filesys/file.h"
#include "lib/kernel/hash.h"
#include "threads/vaddr.h"
#include "vm/pageTable.h"

#define get_zero_byte(READ_BYTE) (PGSIZE - (READ_BYTE % PGSIZE)) % PGSIZE

typedef struct executable_file_elem{
    struct file *file;
    struct hash_elem elem;
    struct lazy_file **lazy_file_list;
    int loaded;
} *executable_file_elem;

void init_exe_hash(void);
void exe_remove (struct executable_file_elem*);
void exe_add (struct executable_file_elem*);
executable_file_elem exe_get_create (struct file*, uint32_t);