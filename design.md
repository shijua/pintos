# PINTOS TASK 0: Alarmclock Design Documents

## PRELIMINARIES

## DATA STRUCTURES

>> A1: (2 marks) 
>> Copy here the declaration of each new or changed `struct` or `struct` member, global or static variable, `typedef', or enumeration.  Identify the purpose of each in roughly 25 words.

```c
struct thread
  {
   /* Owned by thread.c. */
   tid_t tid;                          /* Thread identifier. */
   enum thread_status status;          /* Thread state. */
   char name[16];                      /* Name (for debugging purposes). */
   uint8_t *stack;                     /* Saved stack pointer. */
   int priority;                       /* Priority. */
   struct list_elem allelem;           /* List element for all threads list. */
   int64_t sleep_ticks;                /* Ticks to sleep. */
   /* Shared between thread.c and synch.c. */
   struct list_elem elem;              /* List element. */
    

#ifdef USERPROG
    /* Owned by userprog/process.c. */
    uint32_t *pagedir;                  /* Page directory. */
#endif

    /* Owned by thread.c. */
    unsigned magic;                     /* Detects stack overflow. */
  };
```

I added `int64_t sleep_ticks` in thread so that I can store the ticks when the thread needed to be wake up after block it.

## ALGORITHMS

>> A2: (2 marks)
>> Briefly describe what happens in a call to `timer_sleep()`, including the actions performed by the timer interrupt handler on each timer tick.

When calling `timer_sleep()` function, the Interrupts must be turning on. Afterthat, I firstly check if `ticks <= 0` then it will not sleep and end the function. Otherwise, disable the interrupt first to prevent the thread to be preempted by other kernel threads. Then we calculated out `sleep_ticks` of current thread and insert in into ordered (by `sleep_ticks`) doubly linked list (`block_list`) for wake it up later. Finally we need to block the thread and enable the interrupt again.

Each time calling `timer_interrupt`, old interrupt status will be recorded and then the interrupt being disabled. Sebsequently, we will loop all the element in the `block_list` to check whether the current tick is equals to `sleep_tick`. If it is, then unblock the thread. the interrupt status will be restored at the end of the function.

>> A3: (2 marks)
>> What steps are taken to minimize the amount of time spent in the timer interrupt handler?

When calling `timer_sleep()`, we cost some time on ordering the `block_list` from less to large to help me to cost less time in `timer_interrupt()`.

In `timer_interrupt()`, I am looping through the blocking thread' s `sleep_tick` from less to large (already ordered in `timer_sleep()`). So that the loop can be break if the current thread's `sleep_tick` is larger than current tick. This minimise the amount of speed as I do not checking all the elements in the loop and just some of them need to be blocked. So the latency of the interrupt could be minimised.

## SYNCHRONIZATION

>> A4: (1 mark)
>> How are race conditions avoided when multiple threads call timer_sleep() simultaneously?

I disable the interrupt in `timer_sleep()` and restore when it is finished. So that other thread needs to be waiting when one thread is running.

>> A5: (1 mark)
>> How are race conditions avoided when a timer interrupt occurs during a call to timer_sleep()?

Timer interrupt cannot preempt `timer_sleep()` as the interrupt has been disabled when addingthread to `blocking_list`.

## RATIONALE

>> A6: (2 marks)
>> Why did you choose this design? 
>> In what ways is it superior to another design you considered?

The reason for me to choose this design is that it is relatively easy to implement and it prevents the race conditions on disabling interrupt at some places. This design also can be pretty efficient as it does not contain unused variables and spent minimum time at interrupt.

The first design I think of is just look all the element of `block_list` in `timer_interrupt` everytime. But this idea increase the latency of interrupt as it spent longer time in time interrupt handler. Later on I found out another way that reduce latency by using the ordered list of `block_list` so I do not need to go through all the elements.

