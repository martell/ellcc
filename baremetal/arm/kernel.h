/** Kernel definitions.
 */

#ifndef _kernel_h_
#define _kernel_h_

#include <stdint.h>
#include <stddef.h>

#ifndef NULL
#define NULL 0
#endif

// arm.h
typedef struct context
{
} Context;

/** Set a system call handler.
 * @param nr The system call number.
 * @param fn The system call handling function.
 * @return 0 on success, -1 on  error.
 */
int __set_syscall(int nr, void *fn);

/** Set up a new context.
 * @param savearea Where to put the finished stack pointer.
 * @param entry The context entry point.
 * @param mode The context execution mode.
 * @param ret The context return address.
 * @param arg1 The first argument ro the entry point.
 * @param arg2 The second argument to the entry point.
 */
typedef intptr_t (*ThreadFunction)(intptr_t, intptr_t);

void __new_context(Context **savearea, ThreadFunction entry,
                   int mode, void *ret, intptr_t arg1, intptr_t arg2);

/** Switch to a new context.
 * @param to The new context.
 * @param from A place to store the current context.
 */
void __switch(Context *to, Context **from);

/** Dispatch to a context.
 * @param to The context.
 */
void __dispatch(Context *to);

// sync.h
typedef char Lock;
static inline void lock_aquire(Lock *lock)
{
    // RICH: interrupts.
    while(!__atomic_test_and_set(lock, __ATOMIC_SEQ_CST))
        continue;
}

static inline void lock_release(Lock *lock)
{
    // RICH: interrupts.
    __atomic_clear(lock, __ATOMIC_SEQ_CST);
}

// queue.h
typedef struct entry
{
    struct entry *next;         // The next entry in the queue.
    size_t size;                // The entry size.
} Entry;

struct thread;
typedef struct queue
{
    Entry *head;                // The head of the queue.
    Entry *tail;                // The tail of the queue.
    struct thread *waiter;      // Any threads waiting on the queue.
    Lock lock;
} Queue;

void send_queue(Queue *queue, Entry *entry);
Entry *get_queue_nowait(Queue *queue);
Entry *get_queue(Queue *queue);

// thread.h
typedef struct thread
{
    struct thread *next;        // Next thread in any list.
    Context *saved_sp;          // The thread's saved stack pointer.
} Thread;

/** Create a new thread and make it run-able.
 * @param entry The thread entry point.
 * @param stack The thread stack size.
 * @param arg1 The first parameter.
 * @param arg2 The second parameter.
 * @return The thread ID.
 */
Thread *new_thread(ThreadFunction entry, size_t stack, 
                  intptr_t arg1, intptr_t arg2);

// message.h
typedef struct message
{
    Entry entry;                // Next message in any list.
    int code;                   // The message code.
} Message;

static inline void send_message(Queue *queue, Message *message)
{
    send_queue(queue, (Entry *)message);
}

static inline Message *get_message(Queue *queue)
{
    return (Message *)get_queue(queue);
}

static inline Message *get_message_nowait(Queue *queue)
{
    return (Message *)get_queue_nowait(queue);
}

#endif // _kernel_h_
