#include <bits/syscall.h>       // For syscall numbers.
#define _GNU_SOURCE
#include <sched.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <inttypes.h>
#include "arm.h"
#include "kernel.h"
#include "scheduler.h"


typedef struct thread_queue {
    Thread *head;
    Thread *tail;
} ThreadQueue;

/* The ready_lock protects the following variables..
 */
static Lock ready_lock;

#define IDLE_STACK 4096                 // The idle thread stack size.
#if 0
static ThreadQueue ready[PRIORITIES];

static Thread *current[PROCESSORS];
static Thread idle_thread[PROCESSORS];   // The idle threads.
static char *idle_stack[PROCESSORS][IDLE_STACK];
#endif

static ThreadQueue ready;               // The ready to run list.
static Thread *current;                 // The current running thread.
static long irq_state;                  // Set if an IRQ is active.
/**** End of ready lock protected variables. ****/

static Thread main_thread;              // The main thread.
static Thread idle_thread;              // The idle thread.
static char idle_stack[IDLE_STACK];     // The idle thread stack.


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

/** The idle thread.
 */
static long idle(long arg1, long arg2)
{
    for ( ;; ) {
        // Do stuff, but nothing that will block.
        // ARM should do WFI here.
    }
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
    if (ready.tail) {
        ready.tail->next = thread;
    } else {
        ready.head = thread;
    }
    ready.tail = thread;
    thread->next = NULL;
    thread->state = READY;
}

/* Remove the head of the ready list.
 */
static inline void remove_thread(void)
{
    if (ready.head) {
        ready.head = ready.head->next;
        if (!ready.head) {
            ready.tail = NULL;
        }
    }
}

/** Make the head of the ready list the runniing thread.
 * The ready lock must be aquired before this call.
 */
static void get_running(void)
{
    current = ready.head;
    if (current == NULL) {
        current = &idle_thread;
    } else {
        remove_thread();
    }
    current->state = RUNNING;
    current->next = NULL;
}

/** Change the current thread's state to
 * something besides READY or RUNNING.
 * The ready list must be locked on entry.
 */
static void nolock_change_state(State new_state)
{
    Thread *me = current;
    me->state = new_state;
    get_running();
    __switch(&current->saved_sp, &me->saved_sp);
}

/** Change the current thread's state to
 * something besides READY or RUNNING.
 */
void change_state(State new_state)
{
    lock_aquire(&ready_lock);
    nolock_change_state(new_state);
}

/* Schedule a list of threads.
 */
void schedule(Thread *list)
{
    Thread *next;
    lock_aquire(&ready_lock);

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

    if (irq_state || current == ready.head) {
        // The curent thread continues.
        get_running();
        lock_release(&ready_lock);
        return;
    }

    // Switch to the new thread.
    Thread *me = current;
    get_running();
    __switch(&current->saved_sp, &me->saved_sp);
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
Thread *new_thread(ThreadFunction entry, void *stack, size_t size, 
                   long arg1, long arg2, long r5, long r6, int *status)
{
    char *p;
    if (stack == 0) {
        if (size == 0) size = 4096;     // RICH: #define
        p = malloc(size);

        if (p == NULL) {
            return NULL;
        }
    } else {
        p = stack + size;
    }

    Thread *thread = malloc(sizeof(Thread));    // RICH: bin.
    if (!thread) {
        if (!stack) free(p);
        return NULL;
    }

    thread->next = NULL;
    thread->tls = NULL;
    thread->queue = (MsgQueue)MSG_QUEUE_INITIALIZER;

    thread->saved_sp = (Context *)(p + size);
    (thread->saved_sp - 1)->r5 = r5;
    (thread->saved_sp - 1)->r6 = r6;
    __new_context(&thread->saved_sp, entry, Mode_SYS, arg1, arg2);
    schedule(thread);
    *status = 0;
    return thread;
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
                      long start, long data, long ret)
{       
    int status;
    Thread *new = new_thread((ThreadFunction)ret, stack, 0,
                              0, 0, start, data, &status);
    if (status < 0) {
        return status;
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

/* Initialize the scheduler.
 */
static void init(void)
    __attribute__((__constructor__, __used__));

static void init(void)
{
    // Set up the main and idle threads.
    idle_thread.saved_sp = (Context *)&idle_stack[IDLE_STACK];
    __new_context(&idle_thread.saved_sp, idle, Mode_SYS, 0, 0);
 
    // The main thread is what's running right now.
    ready.head = ready.tail = NULL;
    current = &main_thread;

    // Set up a simple set_tid_address system call.
    __set_syscall(SYS_set_tid_address, sys_set_tid_address);
 
    // Set up the sched_yield system call.
    __set_syscall(SYS_sched_yield, sys_sched_yield);
    // Set up the clone system call.
    __set_syscall(SYS_clone, sys_clone);
}
