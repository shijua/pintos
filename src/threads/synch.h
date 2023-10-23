#ifndef THREADS_SYNCH_H
#define THREADS_SYNCH_H

#include <list.h>
#include <stdbool.h>
#include <list.h>
#include <debug.h>
#define max(a, b) ((a) > (b) ? (a) : (b))
/* A counting semaphore. */
struct semaphore 
  {
    unsigned value;             /* Current value. */
    int max_donation;           /* Max donation value from waiters */
    struct list_elem *max_elem; /* Pointer to max donation elem */
    struct list waiters;        /* List of waiting threads. */
  };

bool thread_priority_less (const struct list_elem *a, const struct list_elem *b, void *aux UNUSED);
bool lock_priority_less (const struct list_elem *a, const struct list_elem *b, void *aux UNUSED);

void sema_init (struct semaphore *, unsigned value);
void sema_down (struct semaphore *);
bool sema_try_down (struct semaphore *);
void sema_up (struct semaphore *);
void sema_self_test (void);

/* Lock. */
struct lock 
  {
    struct thread *holder;      /* Thread holding lock (for debugging). */
    struct semaphore semaphore; /* Binary semaphore controlling access. */
    struct list_elem elem;           /* List element for thread's acquired locks */
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

/* Optimization barrier.

   The compiler will not reorder operations across an
   optimization barrier.  See "Optimization Barriers" in the
   reference guide for more information.*/
#define barrier() asm volatile ("" : : : "memory")

#endif /* threads/synch.h */
