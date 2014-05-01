#ifndef _scheduler_h_
#define _scheduler_h_

#include "kernel.h"

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
    IDLE,                       // This is an idle thread.
    READY,                      // The thread is ready to run.
    RUNNING,                    // The thread is running.
    TIMEOUT,                    // The thread is waiting for a timeout.
    MSGWAIT,                    // The thread is waiting for a message.

    LASTSTATE                   // To get the number of states.
} State;

#if defined(DEFINE_STRINGS)
static const char *state_names[LASTSTATE] =
{
    [IDLE] = "IDLE",
    [READY] = "READY",
    [RUNNING] = "RUNNING",
    [TIMEOUT] = "TIMEOUT",
    [MSGWAIT] = "MSGWAIT",
};
#endif

typedef struct thread
{
    // The saved_sp and tls fields must be first in the thread struct.
    Context *saved_sp;          // The thread's saved stack pointer.
    void *tls;                  // The thread's user space storage.
    struct thread *next;        // Next thread in any list.
    State state;                // The thread's state.
    int priority;               // The thread's priority. 0 is highest.
    MsgQueue queue;             // The thread's message queue.
    const char *name;           // The thread's name.
    struct thread *all_next;    // Next thread in the all thread list.
    struct thread *all_prev;    // Previous thread in the all thread list.
} Thread;

/* Schedule a list of threads.
 */
void schedule(Thread *list);

/** Change the current thread's state to
 * something besides READY or RUNNING.
 */
void change_state(State new_state);

typedef intptr_t (*ThreadFunction)(intptr_t, intptr_t);

/** Switch to a new context.
 * @param to The new context.
 * @param from A place to store the current context.
 */
void __switch(Context **to, Context **from);

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
               void *stack, size_t size, long arg1, long arg2);

/** Get the current threead pointer.
 */
Thread *__get_self(void);

#endif
