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

static Thread *ready;                   // The ready to run list.
static Thread main_thread;              // The main thread.
static Thread idle_thread;              // The idle thread.
#define IDLE_STACK 1024
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
    while (list) {
        next = list->next;
        list->next = ready;
        ready = list;
        list = next;
    }
}

void send_queue(Queue *queue, Entry *entry)
{
    Thread *wakeup = NULL;
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

Entry *get_queue(Queue *queue)
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

Entry *get_queue_wait(Queue *queue)
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
            Thread *me = ready;
            ready = ready->next;
            // Add me to the waiter list.
            me->next = queue->waiter;
            queue->waiter = me;
            lock_release(&queue->lock);
            // Run the next entry in the ready list.
            __switch(&me->saved_sp, ready->saved_sp);
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
