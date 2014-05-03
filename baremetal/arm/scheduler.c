#include <bits/syscall.h>       // For syscall numbers.
#define _GNU_SOURCE
#include <stdio.h>
#include <sched.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <inttypes.h>
#include "arm.h"
#include "timer.h"
#define DEFINE_STRINGS
#include "scheduler.h"
#include "command.h"

#define PRIORITIES 3                    // The number of priorities to support:
                                        // (0..PRIORITIES - 1). 0 is highest.
#define DEFAULT_PRIORITY ((PRIORITIES)/2)
#define PROCESSORS 1                    // The number of processors to support.

#define MIN_STACK   4096                // The minimum stack size for a thread.

typedef struct thread_queue {
    Thread *head;
    Thread *tail;
} ThreadQueue;

#define IDLE_STACK 4096                 // The idle thread stack size.
static Thread idle_thread[PROCESSORS];  // The idle threads.
static char *idle_stack[PROCESSORS][IDLE_STACK];

static void schedule_nolock(Thread *list);
static void insert_all(Thread *thread);

/** The idle thread.
 */
static intptr_t idle(intptr_t arg1, intptr_t arg2)
{
    for ( ;; ) {
        // Do stuff, but nothing that will block.
        // ARM should do WFI here.
    }
}

/** Create an idle thread for each processor.
 */
static void create_idle_threads(void)
{
    for (int i = 0; i < PROCESSORS; ++i) {
        idle_thread[i].saved_sp = (Context *)&idle_stack[i][IDLE_STACK];
        idle_thread[i].priority = PRIORITIES;   // The lowest priority.
        idle_thread[i].state = IDLE;
        char name[20];
        snprintf(name, 20, "idle%d", i);
        idle_thread[i].name = strdup(name);
        insert_all(&idle_thread[i]);
        __new_context(&idle_thread[i].saved_sp, idle, Mode_SYS, 0, 0);
    }
}

/* The ready_lock protects the following variables.
 */
static Lock ready_lock;

static int priority;                    // The current highest priority.

#if PRIORITIES > 1 && PROCESSORS > 1
// Multiple priorities and processors.
static int processor() { return 0; }    // RICH: For now.
static Thread *current[PROCESSORS];
static void *slice_tmo[PROCESSORS];     // Time slice timeout ID.
static ThreadQueue ready[PRIORITIES];   // The ready to run list.
static long irq_state[PROCESSORS];      // Set if an IRQ is active.

#define current current[processor()]
#define idle_thread idle_thread[processor()]
#define slice_tmo slice_tmo[processor()]
#define ready_head(pri) ready[pri].head
#define ready_tail(pri) ready[pri].tail
#define irq_state irq_state[processor()]

#elif PROCESSORS > 1
// Multiple processors, one priority.
static int processor() { return 0; }    // RICH: For now.
static Thread *current[PROCESSORS];
static void *slice_tmo[PROCESSORS];     // Time slice timeout ID.
static ThreadQueue ready;               // The ready to run list.
static long irq_state[PROCESSORS];      // Set if an IRQ is active.

#define current current[processor()]
#define idle_thread idle_thread[processor()]
#define slice_tmo slice_tmo[processor()]
#define ready_head(pri) ready.head
#define ready_tail(pri) ready.tail
#define irq_state irq_state[processor()]

#elif PRIORITIES > 1
// One processor, multiple priorities.
static Thread *current;                 // The current running thread.
static void *slice_tmo;                 // Time slice timeout ID.
static ThreadQueue ready[PRIORITIES];
static long irq_state;                  // Set if an IRQ is active.

#define processor() 0
#define current current
#define idle_thread idle_thread[0]
#define slice_tmo slice_tmo
#define ready_head(pri) ready[pri].head
#define ready_tail(pri) ready[pri].tail

#else
// One processor, one priority.
static Thread *current;                 // The current running thread.
static void *slice_tmo;                 // Time slice timeout ID.
static ThreadQueue ready;               // The ready to run list.
static long irq_state;                  // Set if an IRQ is active.

#define processor() 0
#define current current
#define idle_thread idle_thread[0]
#define slice_tmo slice_tmo
#define ready_head(pri) ready.head
#define ready_tail(pri) ready.tail

#endif

static long long slice_time = 5000000;  // The time slice period (ns).
static ThreadQueue all_threads;         // The all thread list.
/**** End of ready lock protected variables. ****/

static Thread main_thread;              // The main thread.

/** Insert a thread into the all thread list.
 */
void insert_all(Thread *thread)
{
    thread->all_next = NULL;
    all_threads.tail->all_next = thread;
    thread->all_prev = all_threads.tail;
    all_threads.tail = thread;
}

/** Enter the IRQ state.
 */
void *__enter_irq(void)
{
    lock_aquire(&ready_lock);
    long state = irq_state++;
    if (state) return 0;                // Already in IRQ state.
    return current;                     // To save context.
}

void __unlock_ready(void)
{
    lock_release(&ready_lock);
}

/** Leave the IRQ state.
 */
void *__leave_irq(void)
{
    lock_aquire(&ready_lock);
    long state = --irq_state;
    if (state) return 0;                // Still in IRQ state.
    return current;                     // Next context.
}

/** Get the current thread pointer.
 */
Thread *__get_self()
{
    return current;
}

/* Insert a thread in the ready queue.
 */
static inline void insert_thread(Thread *thread)
{
    // A simple FIFO insertion.
    if (ready_tail(thread->priority)) {
        ready_tail(thread->priority)->next = thread;
    } else {
        ready_head(thread->priority) = thread;
    }
    ready_tail(thread->priority) = thread;
    thread->next = NULL;
    thread->state = READY;
}

/** The callback for time slice expiration.
 * @param arg The thread being timed.
 */
static void slice_callback(intptr_t arg1, intptr_t arg2)
{
    lock_aquire(&ready_lock);
    if ((Thread *)arg1 == current) {
        schedule_nolock((Thread *)arg1);
        return;
    }
    lock_release(&ready_lock);
}

/** Make the head of the ready list the runniing thread.
 * The ready lock must be aquired before this call.
 */
static void get_running(void)
{
    int timeslice = 0;
    if (current != ready_head(priority)) {
        // Need to set up for time slicing.
        timeslice = 1;
    }

    current = ready_head(priority);

#if PRIORITIES > 1
    // Check for lower priority things to run.
    while (current == NULL && priority < PRIORITIES) {
        ++priority;
        current = ready_head(priority);
    }
#endif

    if (current == NULL) {
        timeslice = 0;          // No need to timeslice for the idle thread.
        current = &idle_thread;
    } else {
        // Remove the head of the ready list.
        if (ready_head(priority)) {
            ready_head(priority) = ready_head(priority)->next;
            if (!ready_head(priority)) {
                ready_tail(priority) = NULL;
            }
        }
    }
    if (ready_head(priority) == NULL) {
        // No time slicing needed.
        timeslice = 0;
    }

    if(timeslice) {
        // Someone is waiting in the ready queue. Lets be fair.
        if (slice_tmo) {
            timer_cancel_wake_at(slice_tmo);    // Cancel any existing.
        }
        long long when = timer_get_monotonic(); // Get the current time.
        when += slice_time;                     // Add the slice time.
        slice_tmo = timer_wake_at(when, slice_callback, (intptr_t)current, 0);

    }
    current->state = RUNNING;
    current->next = NULL;
}

/* Schedule a list of threads.
 * The ready lock has been aquired.
 */
static void schedule_nolock(Thread *list)
{
    Thread *next;

    // Insert the thread list and the current thread in the ready list.
    int sched_current = 0;
    if (list != current && current != &idle_thread) {
        sched_current = 1;
    }

    while (list) {
        next = list->next;
        insert_thread(list);
        list = next;
    }
    if (sched_current) {
        // Insert the current thread in the ready list.
        insert_thread(current);
    }

    if (irq_state || current == ready_head(current->priority)) {
        // The curent thread continues.
        get_running();
        lock_release(&ready_lock);
        return;
    }

    // Switch to the new thread.
    Thread *me = current;
    if (me == &idle_thread) {
        me->state = IDLE;
    }
    get_running();
    __switch(&current->saved_sp, &me->saved_sp);
}

/* Schedule a list of threads.
 */
void schedule(Thread *list)
{
    lock_aquire(&ready_lock);
    schedule_nolock(list);
}

/** Change the current thread's state to
 * something besides READY or RUNNING.
 * The ready list must be locked on entry.
 */
static int nolock_change_state(State new_state)
{
    Thread *me = current;
    me->state = new_state;
    get_running();
    return __switch(&current->saved_sp, &me->saved_sp);
}

/** Change the current thread's state to
 * something besides READY or RUNNING.
 */
int change_state(State new_state)
{
    lock_aquire(&ready_lock);
    return nolock_change_state(new_state);
}

/* Give up the remaining time slice.
 */
static int sys_sched_yield(void)
{
    schedule(current);
    return 0;
}

/* Create a new thread.
 */
static int new_thread_int(const char *name, Thread **id, ThreadFunction entry, int priority,
                          int cloning, void *stack, size_t size, 
                          intptr_t arg1, intptr_t arg2)
{
    char *p;
    if (stack == 0) {
        if (size < MIN_STACK) {
            size = MIN_STACK;
        }

        p = malloc(size);

        if (p == NULL) {
            return -ENOMEM;
        }

        p += size;
    } else {
        p = stack + size;
    }

    Thread *thread = malloc(sizeof(Thread));    // RICH: bin.
    if (!thread) {
        if (!stack) free(p);
        return -ENOMEM;
    }

    thread->next = NULL;
    thread->tls = NULL;
    if (priority == 0) {
        priority = DEFAULT_PRIORITY;
    } else if (priority >= PRIORITIES) {
        priority = PRIORITIES - 1;
    }
    thread->priority = priority;
    thread->queue = (MsgQueue)MSG_QUEUE_INITIALIZER;
    if (name) {
        thread->name = strdup(name);
    } else {
        thread->name = NULL;
    }
    insert_all(thread);

    Context *cp = (Context *)p;
    thread->saved_sp = cp;
    if (cloning) {
        // Copy registers for clone();
        *(cp - 1) = *current->saved_sp;
    }

    __new_context(&thread->saved_sp, entry, Mode_SYS, arg1, arg2);
    schedule(thread);
    *id = thread;
    return 0;
}

/** Create a new thread and make it run-able.
 * @param name The name of the thread.
 * @param id The new thread ID.
 * @param entry The thread entry point.
 * @param priority The thread priority. 0 is default.
 * @param stack A preallocated stack, or NULL.
 * @param size The stack size.
 * @param arg1 The first parameter.
 * @param arg2 The second parameter.
 * @return 0 on success, < 0 on error.
 */
int new_thread(const char *name, void **id, ThreadFunction entry, int priority,
               void *stack, size_t size, long arg1, long arg2)
{
    Thread *new;
    int s = new_thread_int(name, &new, entry, priority, 
                           0, stack, size, arg1, arg2);
    if (id) {
        *id = new;
    }

    return s;
}

/** Send a message to a message queue.
 */
int send_message(MsgQueue *queue, Message msg)
{
    if (queue == NULL) {
        queue = &__get_self()->queue;
    }
    Envelope *envelope = (Envelope *)malloc(sizeof(Envelope));
    if (!envelope) {
        return -ENOMEM;
    }
    envelope->message = msg;

    Thread *wakeup = NULL;
    envelope->next = NULL;
    lock_aquire(&queue->lock);
    // Queue a envelope.
    if (queue->head) {
        queue->tail->next = envelope;
    } else {
        queue->head = envelope;
    }
    queue->tail = envelope;
    if (queue->waiter) {
        // Wake up sleeping threads.
        wakeup = queue->waiter;
        queue->waiter = NULL;
    }
    lock_release(&queue->lock);
    if (wakeup) {
        // Schedule the sleeping threads.
        schedule(wakeup);
    }
    return 0;
}

Message get_message(MsgQueue *queue)
{
    if (queue == NULL) {
        queue = &__get_self()->queue;
    }

    Envelope *envelope = NULL;;
    do {
        lock_aquire(&queue->lock);
        // Check for queued items.
        if (queue->head) {
            envelope = queue->head;
            queue->head = envelope->next;
            if (!queue->head) {
                queue->tail = NULL;
            }
        }
        if (!envelope) {
            // Sleep until something becomes available.
            // Remove me from the ready list.
            lock_aquire(&ready_lock);
            Thread *me = current;
            // Add me to the waiter list.
            me->next = queue->waiter;
            queue->waiter = me;
            lock_release(&queue->lock);
            nolock_change_state(MSGWAIT);
        } else {
            lock_release(&queue->lock);
        }
    } while(envelope == NULL);

    Message msg = envelope->message;
    free(envelope);
    return msg;
}

Message get_message_nowait(MsgQueue *queue)
{
    if (queue == NULL) {
        queue = &__get_self()->queue;
    }

    Envelope *envelope = NULL;
    lock_aquire(&queue->lock);
    // Check for queued items.
    if (queue->head) {
        envelope = queue->head;
        queue->head = envelope->next;
        if (!queue->head) {
            queue->tail = NULL;
        }
    }
    lock_release(&queue->lock);
    Message msg;
    if (envelope) {
        msg = envelope->message;
        free(envelope);
    } else {
        // No messages available.
        msg  = (Message){ MSG_NONE };
    }
    return msg;
}

/* Set pointer to thread ID.
 * @param tidptr Where to put the thread ID.
 * @return The PID of the calling process.
 *
 * Stub for now.
 */
static long sys_set_tid_address(int *tidptr)
{
    *tidptr = 1;
    return 1;
}

static long sys_clone(unsigned long flags, void *stack, intptr_t *ptid, 
#if defined(__arm__) || defined(__microblaze__) || defined(__ppc__) || \
    defined(__mips__)
                      void *regs, intptr_t *ctid,
#elif defined(__i386__) || defined(__x86_64__)
                      intptr_t *ctid, void *regs,
#else
  #error clone arguments not defined
#endif
                      long arg5, long data, long ret)
{       
    // Create a new thread, copying context.
    static int number = 1;
    char name[20];
    snprintf(name, 20, "clone%d", number++);
    Thread *new;
    int s = new_thread_int(name, &new, (ThreadFunction)ret, 0, 1, stack, 0,
                                 0, 0);
    if (s < 0) {
        return s;
    }

    // Record the TLS.
    new->tls = (void *)data;

    if (flags & CLONE_PARENT_SETTID) {
        VALIDATE_ADDRESS(ptid, sizeof(*ptid), VALID_WR);
        *ptid = (intptr_t)new;
    }

    if (flags & CLONE_CHILD_SETTID) {
        VALIDATE_ADDRESS(ctid, sizeof(*ctid), VALID_WR);
        *ctid = (intptr_t)new;
    }

    return 1;
}

static int tsCommand(int argc, char **argv)
{
    if (argc <= 0) {
        printf("show thread information.\n");
        return COMMAND_OK;
    }

    printf("%8.8s  %-10.10s %5.5s %-10.10s \n", "TID", "STATE", "PRI", "NAME");
    for (Thread *t = all_threads.head; t; t = t->all_next) {
        printf("%8p: ", t);
        printf("%-10.10s ", state_names[t->state]);
        printf("%5d ", t->priority);
        printf("%-10.10s ", t->name ? t->name : "");
        printf("\n");
    }

    return COMMAND_OK;
}

/* Initialize the scheduler.
 */
static void init(void)
    __attribute__((__constructor__, __used__));

static void init(void)
{
    // Set up the main and idle threads.
 
    // The main thread is what's running right now.
    main_thread.priority = DEFAULT_PRIORITY;
    main_thread.state = RUNNING;
    main_thread.next = NULL;
    main_thread.name = "kernel";
    main_thread.all_next = NULL;
    main_thread.all_prev = NULL;
    priority = main_thread.priority;
    all_threads.head = &main_thread;
    all_threads.tail = &main_thread;
    current = &main_thread;

    create_idle_threads();

    // Add the "ts" command.
    command_insert("ts", tsCommand);

    // Set up a simple set_tid_address system call.
    __set_syscall(SYS_set_tid_address, sys_set_tid_address);
 
    // Set up the sched_yield system call.
    __set_syscall(SYS_sched_yield, sys_sched_yield);
    // Set up the clone system call.
    __set_syscall(SYS_clone, sys_clone);
}
