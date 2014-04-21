#include <bits/syscall.h>       // For syscall numbers.
#include <stdlib.h>
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

static Lock ready_lock;
static Thread *ready;                   // The ready to run list.
static Thread *current;                 // The current running thread.

static Thread main_thread;              // The main thread.
static Thread idle_thread;              // The idle thread.
#define IDLE_STACK 4096
static char *idle_stack[IDLE_STACK];    // The idle thread stack.

/** The idle thread.
 */
static intptr_t idle(intptr_t arg1, intptr_t arg2)
{
    for ( ;; ) {
        // Do stuff, but nothing that will block.
    }
}

/* A bare-bones scheduler. Just shove threads onto the ready list.
 */
void schedule(Thread *list)
{
    Thread *next;
    lock_aquire(&ready_lock);
    while (list) {
        next = list->next;
        list->next = ready;
        ready = list;
        list = next;
    }

    if (current == ready) {
        // The curent thread continues.
        lock_release(&ready_lock);
        return;
    }

    // Switch to the new thread.
    Thread *me = current;
    current = ready;
    __switch(ready->saved_sp, &me->saved_sp, lock_release, &ready_lock);
}

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
            Thread *me = ready;
            current = ready = me->next;
 
            // Add me to the waiter list.
            me->next = queue->waiter;
            queue->waiter = me;
            lock_release(&queue->lock);
            // Run the next entry in the ready list.
            __switch(ready->saved_sp, &me->saved_sp, lock_release, &ready_lock);
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
    main_thread.next = &idle_thread;
    current = ready = &main_thread;

    // Set up a simple set_tid_address system call.
    __set_syscall(SYS_set_tid_address, sys_set_tid_address);
}
