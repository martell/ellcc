/** Scheduler definitions.
 */
#ifndef _scheduler_h_
#define _scheduler_h_

#include <sys/types.h>
#include "kernel.h"

#define HAVE_CAPABILITY 1

#if !defined(HAVE_CAPABILITY)
// Define a simple capability check.
#define CAPABLE(thread, capability) \
  (!(thread)->euid)

#else

// Allow changing file and group ownership.
#define CAP_CHOWN 0

// Override DAC restrictions.
#define CAP_DAC_OVERRIDE 1

// Override DAC restrictions on read and search on files and directories.
#define CAP_DAC_READ_SEARCH 2

// Override file owenership restrictions except where CAP_FSETID is applicable.
#define CAP_FOWNER 3

// Override set[ug]id restrictions on files.
#define CAP_FSETID 4

// Override restrictions on sending signals.
#define CAP_KILL 5

// Override gid manipulation restrictions.
#define CAP_SETGID 6

// Override uid manipulation restrictions.
#define CAP_SETUID 7

// Set all capabilities for the superuser.
#define SUPERUSER_CAPABILITIES (~(capability_t)0)
#define NO_CAPABILITIES ((capability_t)0)

#define CAPABILITY_TO_BIT(capability) (1 << (capability))
#define CAPABLE(thread, capability) \
  ((thread)->ecap & CAPABILITY_TO_BIT(capability))

typedef int capability_t;

#endif

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
  EXITING,                      // The thread is exiting.
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
  const char *name;             // The thread's name.
  int *set_child_tid;           // The set child thread id address.
  int *clear_child_tid;         // The clear child thread id address.
  pid_t pid;                    // The process id.
  pid_t tid;                    // The thread id.
  pid_t ppid;                   // The thread's parent id.
  uid_t uid;                    // The thread's user id.
  uid_t euid;                   // The thread's effective user id.
  uid_t suid;                   // The thread's saved user id.
  uid_t fuid;                   // The thread's file user id.
  gid_t gid;                    // The thread's group id.
  gid_t egid;                   // The thread's effective group id.
  gid_t sgid;                   // The thread's saved group id.
  gid_t fgid;                   // The thread's file group id.
  pid_t pgid;                   // The thread's process group.
  pid_t sid;                    // The thread's session id.
#if defined(HAVE_CAPABILITY)
  capability_t cap;             // The thread's capabilities.
  capability_t ecap;            // The thread's effective capabilities.
  capability_t icap;            // The thread's inheritable capabilities.
#endif
  MsgQueue queue;               // The thread's message queue.
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

/** Enter a new context.
 * @param to The new context.
 */
int __elk_enter(__elk_context **to);

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
