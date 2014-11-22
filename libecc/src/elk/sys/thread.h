/** Thread definitions.
 */
#ifndef _thread_h_
#define _thread_h_

#include <limits.h>
#include "config.h"
#include "file.h"
#include "kernel.h"

#if ELK_NAMESPACE
#define capable(arg) __elk_capable(arg)
#define lock_aquire __elk_lock_aquire
#define lock_release __elk_lock_release
#define send_message_q __elk_send_message_q
#define send_message __elk_send_message
#define timer_expired __elk_timer_expired
#define timer_wake_at __elk_timer_wake_at
#define getfile __elk_getfile
#define getdup __elk_getdup
#define allocfd __elk_allocfd
#define setfile __elk_setfile
#define replacecwd __elk_replacecwd
#define getpath __elk_getpath
#define vm_allocate __elk_vm_allocate
#define vm_free __elk_vm_free
#endif

#if !HAVE_CAPABILITY
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

// Allow raw I/O.
#define CAP_SYS_RAWIO 17

// Allow mount, umount.
#define CAP_SYS_ADMIN 21

#define CAPABILITY_TO_BIT(capability) (1 << (capability))

typedef int capability_t;

int capable(capability_t cap);

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

typedef struct
{
  int lock;
  int level;
} lock_t;

#define LOCK_INITIALIZER { 0, 0 }

static inline void lock_aquire(lock_t *lock)
{
// RICH:
#if !defined(__microblaze__)
  while(!__atomic_test_and_set(&lock->lock, __ATOMIC_SEQ_CST))
      continue;
#endif
  lock->level = splhigh();
}

static inline void lock_release(lock_t *lock)
{
  splx(lock->level);
// RICH:
#if !defined(__microblaze__)
  __atomic_clear(&lock->lock, __ATOMIC_SEQ_CST);
#endif
}

typedef struct queue
{
  lock_t lock;
  Envelope *head;               // The head of the queue.
  Envelope *tail;               // The tail of the queue.
  struct thread *waiter;        // Any threads waiting on the queue.
} MsgQueue;

#define MSG_QUEUE_INITIALIZER { LOCK_INITIALIZER, NULL, NULL, NULL }

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
int send_message_q(MsgQueue *queue, Message message);

/** Send a message to a message queue.
 * @param queue The message queue.
 * @param message The message to send.
 * @return 0 on success, else -errno.
 */
int send_message(int tid, Message message);

Message get_message(MsgQueue *queue);
Message get_message_nowait(MsgQueue *queue);

// Thread states.
typedef enum state {
  IDLE,                         // This is an idle thread.
  READY,                        // The thread is ready to run.
  RUNNING,                      // The thread is running.
  EXITING,                      // The thread is exiting.
  SLEEPING,                     // The thread is sleeping.
  MSGWAIT,                      // The thread is waiting for a message.

  LASTSTATE                     // To get the number of states.
} state;

#if defined(DEFINE_STATE_STRINGS)
static const char *state_names[LASTSTATE] =
{
  [IDLE] = "IDLE",
  [READY] = "READY",
  [RUNNING] = "RUNNING",
  [SLEEPING] = "SLEEPING",
  [MSGWAIT] = "MSGWAIT",
};
#endif

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

/** Timer expired handler.
 * This function is called in an interrupt context.
 */
long long timer_expired(long long when);

/** Make an entry in the sleeping list and sleep
 * or schedule a callback.
 */
typedef void (*TimerCallback)(void *, void *);
void *timer_wake_at(long long when,
                    TimerCallback callback, void *arg1, void *arg2, int retval);

struct file;

/** Get a file pointer corresponding to a file descriptor.
 */
int getfile(int fd, struct file **filep);

/** Get a duplicate file descriptor.
 */
int getdup(int fd, struct file **filep, int free);

/** Get a file descriptor.
 */
int allocfd(file_t fp);

/** Set a file pointer corresponding to a file descriptor.
 */
int setfile(int fd, struct file *file);

/** Replace the old cwd fp with a new one.
 */
file_t replacecwd(file_t fp);

/** Get a file path.
 * This function returns the full path name for the file name.
 */
int getpath(const char *name, char *path);

/** Get the current thread id.
 */
int gettid(void);

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif
#define PAGE_MASK (~(PAGE_SIZE-1))
#define round_page(x) (((x) + PAGE_MASK) & ~PAGE_MASK)

/** Allocate and free memory.
 */
int vm_allocate(void **addr, size_t size, int anywhere);
int vm_free(void *addr);

#endif
