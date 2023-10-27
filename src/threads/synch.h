#ifndef THREADS_SYNCH_H
#define THREADS_SYNCH_H

#include <list.h>
#include <stdbool.h>
#include <debug.h>
#define getLock(LOCK_ELEM) list_entry(LOCK_ELEM, struct lock, elem)
/* A counting semaphore. */
struct semaphore 
  {
    int8_t max_donation;
    unsigned value;             /* Current value. */
    struct list waiters;        /* List of waiting threads. */
  };

void sema_init (struct semaphore *, unsigned value);
void sema_down (struct semaphore *);
bool sema_try_down (struct semaphore *);
void sema_up (struct semaphore *);
void sema_self_test (void);

/* Lock. */
struct lock 
  {
    struct list_elem elem;      /* elements that used to build a 
                                    sorted list by max_donation */
    struct thread *holder;      /* Thread holding lock (for debugging). */
    struct semaphore semaphore; /* Binary semaphore controlling access. */
  };

void lock_init (struct lock *);
void lock_acquire (struct lock *);
bool lock_try_acquire (struct lock *);
void lock_release (struct lock *);
bool lock_held_by_current_thread (const struct lock *);

/* Condition variable. */
struct condition 
  {
    struct list waiters;        /* List of waiting semaphore_elems. */
  };

void cond_init (struct condition *);
void cond_wait (struct condition *, struct lock *);
void cond_signal (struct condition *, struct lock *);
void cond_broadcast (struct condition *, struct lock *);

void reset_lock_donation (struct semaphore *);
void sema_priority_from_acquires (struct thread *);

// compare the maximum priority of the locks, thread and sema
list_less_func lock_priority_less;
list_less_func thread_priority_less;
list_less_func sema_elem_less;

/* Optimization barrier.

   The compiler will not reorder operations across an
   optimization barrier.  See "Optimization Barriers" in the
   reference guide for more information.*/
#define barrier() asm volatile ("" : : : "memory")

#endif /* threads/synch.h */
