/** Scheduler definitions.
 */
#ifndef _scheduler_h_
#define _scheduler_h_

#include "kernel.h"

// RICH: Should messages go away?
typedef struct message
{
  int code;                     // The message code.
                                // Other stuff can be added.
} Message;

typedef struct envelope {
  struct envelope *next;
  Message message;
} Envelope;

struct __elk_thread;
typedef struct queue
{
  Envelope *head;               // The head of the queue.
  Envelope *tail;               // The tail of the queue.
  struct __elk_thread *waiter;  // Any threads waiting on the queue.
  __elk_lock lock;
} MsgQueue;

#define MSG_QUEUE_INITIALIZER { NULL, NULL, NULL, LOCK_INITIALIZER }

/** System message codes.
 */
enum {
  MSG_NONE,                     // No message.
  MSG_TIMEOUT,                  // A timeout has occured.
};

/** Send a message to a message queue.
 * @param queue The message queue.
 * @param message The message to send.
 * @return 0 on success, else -errno.
 */
int __elk_send_message_q(MsgQueue *queue, Message message);

/** Send a message to a message queue.
 * @param queue The message queue.
 * @param message The message to send.
 * @return 0 on success, else -errno.
 */
int __elk_send_message(int tid, Message message);

Message get_message(MsgQueue *queue);
Message get_message_nowait(MsgQueue *queue);

// Thread states.
typedef enum state {
  IDLE,                         // This is an idle thread.
  READY,                        // The thread is ready to run.
  RUNNING,                      // The thread is running.
  TIMEOUT,                      // The thread is waiting for a timeout.
  SEMWAIT,                      // The thread is waiting on a semaphore.
  SEMTMO,                       // The thread is waiting on a semaphore
                                //     with a timeout.
  MSGWAIT,                      // The thread is waiting for a message.

  LASTSTATE                     // To get the number of states.
} __elk_state;

#if defined(DEFINE_STRINGS)
static const char *state_names[LASTSTATE] =
{
  [IDLE] = "IDLE",
  [READY] = "READY",
  [RUNNING] = "RUNNING",
  [TIMEOUT] = "TIMEOUT",
  [SEMWAIT] = "SEMWAIT",
  [SEMTMO] = "SEMTMO",
  [MSGWAIT] = "MSGWAIT",
};
#endif

typedef struct __elk_thread
{
  // The saved_ctx and tls fields must be first in the thread struct.
  __elk_context *saved_ctx;     // The thread's saved context.
  void *tls;                    // The thread's user space storage.
  struct __elk_thread *next;    // Next thread in any list.
  __elk_state state;            // The thread's state.
  int priority;                 // The thread's priority. 0 is highest.
  MsgQueue queue;               // The thread's message queue.
  const char *name;             // The thread's name.
  int pid;                      // The process id.
  int tid;                      // The thread id.
  int *set_child_tid;           // The set child thread id address.
  int *clear_child_tid;         // The clear child thread id address.
} __elk_thread;

/* Schedule a list of threads.
 */
void __elk_schedule(__elk_thread *list);

/** Change the current thread's state to
 * something besides READY or RUNNING.
 * @param arg The tennative value returned. 
 * @param state Then new state to enter. 
 */
int __elk_change_state(int arg, __elk_state new_state);

/** Switch to a new context.
 * @param to The new context.
 * @param from A place to store the current context.
 */
int __elk_switch(__elk_context **to, __elk_context **from);

/** Switch to a new context.
 * @param arg The tenative return value when the context is restarted.
 * @param to The new context.
 * @param from A place to store the current context.
 */
int __elk_switch_arg(int arg, __elk_context **to, __elk_context **from);

/** Set up a new context.
 * @param savearea Where to put the finished stack pointer.
 * @param entry The context entry point (0 if return to caller).
 * @param mode The context execution mode.
 * @param arg1 The first argument to the entry point.
 * @param arg2 The second argument to the entry point.
 * @return 1 to indicate non-clone, else arg1.
 */

int __elk_new_context(__elk_context **savearea, void (entry)(void),
                      int mode, long arg);

/** Get the current thread pointer.
 */
__elk_thread *__elk_thread_self(void);

/** futex operations.
 */
#define FUTEX_WAIT              0
#define FUTEX_WAKE              1
#define FUTEX_FD                2
#define FUTEX_REQUEUE           3
#define FUTEX_CMP_REQUEUE       4
#define FUTEX_WAKE_OP           5
#define FUTEX_LOCK_PI           6
#define FUTEX_UNLOCK_PI         7
#define FUTEX_TRYLOCK_PI        8
#define FUTEX_WAIT_BITSET       9
#define FUTEX_CLOCK_REALTIME    256

/** Get the current thread id.
 */
int gettid(void);

#endif
