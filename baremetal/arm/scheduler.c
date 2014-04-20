#include "arm.h"
#include "kernel.h"
#include <stdlib.h>

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
static Thread main_thread;              // The main thread.
static Thread idle_thread;              // The idle thread.
#define IDLE_STACK 4096
static char *idle_stack[IDLE_STACK];    // The idle thread stack.

static intptr_t idle(intptr_t arg1, intptr_t arg2)
{
    for ( ;; ) {
    }
}

/* Initialize the scheduler.
 */
static void init(void)
    __attribute__((__constructor__, __used__));

static void init(void)
{
        idle_thread.saved_sp = (Context *)&idle_stack[IDLE_STACK];
        __new_context(&idle_thread.saved_sp, idle, Mode_SYS, NULL,
                      0, 0);
        main_thread.next = &idle_thread;
        ready = &main_thread;
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
    lock_release(&ready_lock);
    // RICH: hole here. Keep interrupts disabled? SMP?
    __switch(ready->saved_sp, &ready->next->saved_sp);
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
            ready = me->next;
            lock_release(&ready_lock);
 
            // Add me to the waiter list.
            me->next = queue->waiter;
            queue->waiter = me;
            lock_release(&queue->lock);
            // Run the next entry in the ready list.
            __switch(ready->saved_sp, &me->saved_sp);
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
