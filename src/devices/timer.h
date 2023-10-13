#ifndef DEVICES_TIMER_H
#define DEVICES_TIMER_H

#include <round.h>
#include <stdint.h>
#include "lib/kernel/list.h"      /* used for sleep list         */
#include "threads/synch.h"        /* used for semaphore and lock */

/* Number of timer interrupts per second. */
#define TIMER_FREQ 100

void timer_init (void);
void timer_calibrate (void);

int64_t timer_ticks (void);
int64_t timer_elapsed (int64_t);

/* Sleep and yield the CPU to other threads. */
void timer_sleep (int64_t ticks);
void timer_msleep (int64_t milliseconds);
void timer_usleep (int64_t microseconds);
void timer_nsleep (int64_t nanoseconds);

/* thread with a semaphore */
struct sleep_list_thread {
    struct thread *sleep_thread;
    struct semaphore *sema;
    struct list_elem list_e;
    int64_t sleep_ticks;
};

/* prototype for sleep_thread_less_func */
bool sleep_thrad_less_func (const struct list_elem *, const struct list_elem
                            *, void *);

/* Busy waits. */
void timer_mdelay (int64_t milliseconds);
void timer_udelay (int64_t microseconds);
void timer_ndelay (int64_t nanoseconds);

void timer_print_stats (void);

#endif /* devices/timer.h */
