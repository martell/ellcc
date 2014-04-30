/** Kernel definitions.
 */

#ifndef _kernel_h_
#define _kernel_h_

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "arm.h"

#ifndef NULL
#define NULL 0
#endif

// sync.h
typedef struct lock
{
    char lock;
    int level;
} Lock;

#define LOCK_INITIALIZER { 0, 0 }

static inline void lock_aquire(Lock *lock)
{
    while(!__atomic_test_and_set(&lock->lock, __ATOMIC_SEQ_CST))
        continue;
    lock->level = splhigh();
}

static inline void lock_release(Lock *lock)
{
    splx(lock->level);
    __atomic_clear(&lock->lock, __ATOMIC_SEQ_CST);
}

typedef struct message
{
    int code;                   // The message code.
                                // Other stuff can be added.
} Message;

typedef struct envelope {
    struct envelope *next;
    Message message;
} Envelope;

struct thread;
typedef struct queue
{
    Envelope *head;             // The head of the queue.
    Envelope *tail;             // The tail of the queue.
    struct thread *waiter;      // Any threads waiting on the queue.
    Lock lock;
} MsgQueue;

#define MSG_QUEUE_INITIALIZER { NULL, NULL, NULL, LOCK_INITIALIZER }

/** System message codes.
 */
enum {
    MSG_NONE,                   // No message.
    MSG_TIMEOUT,                // A timeout has occured.
};

/** Send a message to a message queue.
 * @param queue The message queue.
 * @param message The message to send.
 * @return 0 on success, else -errno.
 */
int send_message(MsgQueue *queue, Message message);

Message get_message(MsgQueue *queue);
Message get_message_nowait(MsgQueue *queue);

// thread.h

// Thread states.
typedef enum state {
    READY,                      // The thread is ready to run.
    RUNNING,                    // The thread is running.
    TIMEOUT,                    // The thread is waiting for a timeout.
    MSGWAIT,                    // The thread is waiting for a message.
} State;

typedef struct thread
{
    // The saved_sp and tls fields must be first in the thread struct.
    Context *saved_sp;          // The thread's saved stack pointer.
    void *tls;                  // The thread's user space storage.
    struct thread *next;        // Next thread in any list.
    State state;                // The thread's state.
    int priority;               // The thread's priority. 0 is highest.
    MsgQueue queue;             // The thread's message queue.
} Thread;

/** Change the current thread's state to
 * something besides READY or RUNNING.
 */
void change_state(State new_state);

/* Schedule a list of threads.
 */
void schedule(Thread *list);

typedef long (*ThreadFunction)(long, long);

/** Switch to a new context.
 * @param to The new context.
 * @param from A place to store the current context.
 */
void __switch(Context **to, Context **from);

/* RICH: Validate a system call address argument.
 */
enum {
    VALID_RD = 0x01,
    VALID_WR = 0x02,
    VALID_RW = 0x03,
    VALID_EX = 0x04
};

#define VALIDATE_ADDRESS(addr, size, access)

/** Set a system call handler.
 * @param nr The system call number.
 * @param fn The system call handling function.
 * @return 0 on success, -1 on  error.
 */
int __set_syscall(int nr, void *fn);

/** Set up a new context.
 * @param savearea Where to put the finished stack pointer.
 * @param entry The context entry point (0 if return to caller).
 * @param mode The context execution mode.
 * @param arg1 The first argument to the entry point.
 * @param arg2 The second argument to the entry point.
 * @return 1 to indicate non-clone, else arg1.
 */

int __new_context(Context **savearea, ThreadFunction entry, int mode,
                  long arg1, long arg2);
/** Create a new thread and make it run-able.
 * @param entry The thread entry point.
 * @param priority The thread priority. 0 is default.
 * @param stack A preallocated stack, or NULL.
 * @param size The stack size.
 * @param arg1 The first parameter.
 * @param arg2 The second parameter.
 * @param status A place to put any generated errno values.
 * @return The thread ID.
 */
Thread *new_thread(ThreadFunction entry, int priority,
                   void *stack, size_t size, 
                   long arg1, long arg2, long r5, long r6, int *status);

/** Get the current threead pointer.
 */
Thread *__get_self(void);

#endif // _kernel_h_
