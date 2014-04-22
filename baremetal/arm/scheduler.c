#include <bits/syscall.h>       // For syscall numbers.
#include <stdlib.h>
#include <errno.h>
#include "arm.h"
#include "kernel.h"

// scheduler.h
struct ready_msg
{
    Message header;
    Thread *thread;
};
extern Queue scheduler_queue;
typedef enum {
    SCHEDULER_READY
} SchedulerCode;

Queue scheduler_queue;

/* The ready_lock protects both the ready to run list and the current thread.
 */
static Lock ready_lock;

typedef struct thread_queue {
    Thread *head;
    Thread *tail;
} ThreadQueue;

#if 0
static ThreadQueue ready[PRIORITIES];

static Thread *current[PROCESSORS];
static Thread idle_thread[PROCESSORS];   // The idle threads.
static char *idle_stack[PROCESSORS][IDLE_STACK];
#endif

static ThreadQueue ready;               // The ready to run list.
static Thread *current;                 // The current running thread.

static Thread main_thread;              // The main thread.
static Thread idle_thread;              // The idle thread.
#define IDLE_STACK 4096
static char *idle_stack[IDLE_STACK];    // The idle thread stack.

/** The idle thread.
 */
#include <stdio.h>
static intptr_t idle(intptr_t arg1, intptr_t arg2)
{
    for ( ;; ) {
        // Do stuff, but nothing that will block.
    }
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

/* Schedule a list of threads.
 */
void schedule(Thread *list)
{
    Thread *next;
    lock_aquire(&ready_lock);

    // Insert the thread list and the current thread in the ready list.
    current->next = list;
    list = current;

    while (list) {
        next = list->next;
        insert_thread(list);
        list = next;
    }

    if (current == ready.head) {
        remove_thread();
        current->next = NULL;
        // The curent thread continues.
        lock_release(&ready_lock);
        return;
    }

    // Switch to the new thread.
    Thread *me = current;
    current = ready.head;
    if (current == NULL) {
        current = &idle_thread;
    } else {
        remove_thread();
    }
    current->next = NULL;
    __switch(current->saved_sp, &me->saved_sp, lock_release, &ready_lock);
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
Thread *new_thread(ThreadFunction entry, size_t stack, 
    intptr_t arg1, intptr_t arg2)
{
    Thread *thread = malloc(sizeof(Thread));    // RICH: bin.
    if (!thread) return NULL;
    thread->next = NULL;
    if (stack == 0) stack = 4096;
    char *p = malloc(stack);
    if (p == NULL) {
        free(thread);
        return NULL;
    }
    thread->saved_sp = (Context *)(p + stack);
    __new_context(&thread->saved_sp, entry, Mode_SYS, NULL, arg1, arg2);
    schedule(thread);
    return thread;
}

void send_queue(Queue *queue, Entry *entry)
{
    Thread *wakeup = NULL;
    entry->next = NULL;
    lock_aquire(&queue->lock);
    // Queue a message.
    if (queue->head) {
        queue->tail->next = entry;
    } else {
        queue->head = entry;
    }
    queue->tail = entry;
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
}

Entry *get_queue_nowait(Queue *queue)
{
    Entry *entry = NULL;
    lock_aquire(&queue->lock);
    // Check for queued items.
    if (queue->head) {
        entry = queue->head;
        queue->head = entry->next;
        if (!queue->head) {
            queue->tail = NULL;
        }
    }
    lock_release(&queue->lock);
    return entry;
}

Entry *get_queue(Queue *queue)
{
    Entry *entry = NULL;
    do {
        lock_aquire(&queue->lock);
        // Check for queued items.
        if (queue->head) {
            entry = queue->head;
            queue->head = entry->next;
            if (!queue->head) {
                queue->tail = NULL;
            }
        }
        if (!entry) {
            // Sleep until something becomes available.
            // Remove me from the ready list.
            lock_aquire(&ready_lock);
            Thread *me = current;
            current = ready.head;
            if (current == NULL) {
                current = &idle_thread;
            } else {
                remove_thread();
            }
            current->next = NULL;
 
            // Add me to the waiter list.
            me->next = queue->waiter;
            queue->waiter = me;
            lock_release(&queue->lock);
            // Run the next entry in the ready list.
            __switch(current->saved_sp, &me->saved_sp, lock_release, &ready_lock);
        } else {
            lock_release(&queue->lock);
        }
    } while(entry == NULL);
    return entry;
}

void scheduler(int saved)
{
    Message *message = get_message(&scheduler_queue);
    if (message) {
    }
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

static int sys_clone(unsigned long flags, void *stack, void *ptid, 
#if defined(__arm__) || defined(__microblaze__) || defined(__ppc__) || \
    defined(__mips__)
                     void *regs, void *ctid
#elif defined(__i386__) || defined(__x86_64__)
                     void *ctid, void *regs
#else
  #error clone arguemnts not defined
#endif
                    )
{       
    printf("in clone\n");
    return -ENOSYS;
}

/* Initialize the scheduler.
 */
static void init(void)
    __attribute__((__constructor__, __used__));

static void init(void)
{
    // Set up the main and idle threads.
    idle_thread.saved_sp = (Context *)&idle_stack[IDLE_STACK];
    __new_context(&idle_thread.saved_sp, idle, Mode_SYS, NULL,
                  0, 0);
 
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
