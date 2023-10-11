## question 1

> Which Git command should you run to retrieve a copy of your individual repository for Pintos Task 0 in your local directory?

using cammand `git clone https://gitlab.doc.ic.ac.uk/lab2324_autumn/pintos_task0_<user_name>.git`

for example: `git clone https://gitlab.doc.ic.ac.uk/lab2324_autumn/pintos_task0_wh1322.git`

## question 2

> Why is using the `strcpy()` function to copy strings usually a bad idea?

When you use this function it may cause the output string overflow if output string has a smaller size.  It would be probably better to use `strlcpy` instead.

## question 3 

> If test `src/tests/devices/alarm-multiple` fails, where would you find its output and result logs? Provide both paths and filenames.

We can find logs in directory of `src/devices/build/tests/devices`: the results for `alarm-mutiple` are  `alarm-multiple.output` for output and  `alarm-multiple.result` for result.

## question 4

> In Pintos, a thread is characterized by a struct and an execution stack.

> (a) What are the limitations on the size of these data structures?

The size of `struct thread` should be stay under 1KB. `stack` needs to be under 3KB insize.

> (b) Explain how this relates to stack overflow and how Pintos identifies if a stack overflow has occurred.

Stack overflow happens when the stack is full(exceed 4 KB). When the stack overflows, the thread state will be affected hance pintos can detect it. There is a varible in thread called `magic` is used to detect overflow. The value will be changed when overflow happens.

## question 5

> Explain how thread scheduling in Pintos currently works in roughly 300 words. Include the chain of execution of function calls.

Threads would be created by using struct thread. Each struct set will occupy a part of memory  in page and the rest part would act as stack. Each thread will have a `tid` which is unique. Thread have differnet status ranging from `THREAD_RUNNING` to `THREAD_READY` to `THREAD_BLOCKED` and `THREAD_DYING`. `THREAD_RUNNING` indicate that it is currently running. It also contain `name`, `stack` pointer, `priority`, `allelem`, `elem` and `magic`.  `stack` is used for storing registrs and `intr_frame` if interrupt occurs. `priority` is used for helping schedulers to choose which thread to go with. `allelem` is a list of all thread and `elem` is showing as a doubly lisked list. `magic` number is a variable used for detecting stack overflow.

Firstly, in the `main` function, it will call `thread_init()` to initialise the thread system. Then `thread_start()` will be called to start the schedule. It will also create a idle thread while being called. While using the thread, the system can call `thread_create()` to create a new thread. System can call `thread_current()` to get current thread status and `thread_block()` and `thread_unblock()` to change thread status.

For switch thread, interrupt is send and `schedule()` is used. This function records current thread, determines which function will be running next(`by calling 1next_thread_to_run()`), and calling `switch_threads()`. The rest of scheduler is implemented in `thread_schedule_taik()` and will frees the whole page of thread if it is in dying state(meaning that the thread is ended).

Interrupt is highly important in threads. Without interrupt, Unexptected behaviour would be happened when program missed some input or when two thread want to dealing with the same input. It ensures that thread working correctly.

## question 6

> In Pintos, what is the default length (in ticks and in seconds) of a scheduler time slice?

there are `TIME_SLICE` ticks per time slice. The default is 4 ticks(100 ticks per second).

## question 7

> In Pintos, how would you print an unsigned 64 bit int? (Consider that you are working with C99). Don’t forget to state any inclusions needed by your code.

`<inttypes.h>` needs to be included. then we can print like `printf("%" PRIu64 "\n", my_int);`

## question 8

> Explain the property of reproducibility and how the lack of reproducibility will affect debugging.

reproducibility allows programmar to reuse their code. The code can be use multiple times so that less lines of code are written hence easier to read. This helps debugging as programmar can fetch the error out easier. If they write multiple lines of code with same error they need to modify same thing multiple times, leading to more time on debugging.

## question 9

> In Pintos, locks are implemented on top of semaphores.

> (a) How do the functions in the API of locks relate to those of semaphores?

Lock's `release` is similar to `up` and `acquire` is similare to `down`.

> (b) What extra property do locks have that semaphores do not?

Lock is similar to semapore but lock has a restriction that only the thread the requires the lock can allow to release it.

## question 10

> Define what is meant by a race-condition. Why is the test `if(x \!= null)` insufficient to prevent a segmentation fault from occurring on an attempted access to a structure through the pointer x?

Race condition is happening when multiple thread execute on the same input and leads to a different behaviour.  Even if x is not null at that moment, the address will be deleted or used by other thread at the same time, so segamentation fault may still happens. (segenmentation fault is a kind of error accessing the memory that is not belong to you)
