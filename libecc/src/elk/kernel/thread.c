/** Schedule threads for execution.
 */
#include <syscalls.h>                   // For syscall numbers.
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/socket.h>
#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <sched.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <inttypes.h>

#include "config.h"                     // Configuration parameters.
#include "timer.h"
#define DEFINE_STATE_STRINGS
#include "thread.h"
#include "file.h"
#include "vnode.h"
#include "page.h"
#include "kmem.h"
#include "command.h"


#if ENABLEFDS
#include <stdarg.h>
#endif

// Make threads a loadable feature.
FEATURE(thread)

#if HAVE_CAPABILITY
// Set all capabilities for the superuser.
#define SUPERUSER_CAPABILITIES (~(capability_t)0)
#define NO_CAPABILITIES ((capability_t)0)

#define CAPABLE(thread, capability) \
  ((thread)->ecap & CAPABILITY_TO_BIT(capability))

#define DEFAULT_PRIORITY ((PRIORITIES)/2)
#endif

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

/** A thread is an indepenent executable context in ELK.
 * A thread is identified by a unique identifier, the thread id (tid).
 * Threads are grouped in to processes, identified by a process id (pid).
 * The tid of the first thread in a process is the pid of a process.
 * All threads have a parent id (ppid), which is the tid of the thread
 * that created them.
 * The threads in a process share all accessed system resources, e.g.
 * memory space, file descriptors, etc. but they each have an independent
 * thread structure, defined below, and each thread has a separate stack.
 * The first process in ELK (with pid == 0) is the kernel process.
 * Each thread has a user id (uid) associated with it. Except for uid
 * 0, which in the Unix traditional is the "root" user, uids are arbitrary.
 */

struct robust_list_head
{
  volatile void *volatile head;
  long off;
  volatile void *volatile pending;
};

typedef struct thread
{
  // The saved_ctx and tls fields must be first in the thread struct.
  context_t *saved_ctx;         // The thread's saved context.
  void *tls;                    // The thread's user space storage.
  struct thread *next;          // Next thread in any list.
  state state;                  // The thread's state.
  unsigned flags;               // Flags associated with this thread.
  int priority;                 // The thread's priority. 0 is highest.
  const char *name;             // The thread's name.
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
  mode_t umask;                 // The file creation mask.
#if HAVE_CAPABILITY
  capability_t cap;             // The thread's capabilities.
  capability_t ecap;            // The thread's effective capabilities.
  capability_t icap;            // The thread's inheritable capabilities.
#endif
#if ENABLEFDS
  fdset_t fdset;                // File descriptors used by the thread.
  file_t cwdfp;                 // The current working directory.
#endif
#if HAVE_VM
  vm_map_t map;                 // The process memory map.
#endif
  // A pointer to the user space robust futex list.
  struct robust_list_head *robust_list;
  struct queue queue;           // The thread's message queue.
} thread_t;

static int timer_cancel_wake_at(void *id);

/** Switch to a new context.
 * @param to The new context.
 * @param from A place to store the current context.
 * This function is implemented in crt1.S.
 */
int __elk_switch(context_t **to, context_t **from);

/** Switch to a new context.
 * @param arg The tenative return value when the context is restarted.
 * @param to The new context.
 * @param from A place to store the current context.
 * This function is implemented in crt1.S.
 */
int __elk_switch_arg(int arg, context_t **to, context_t **from);

/** Enter a new context.
 * @param to The new context.
 * This function is implemented in crt1.S.
 */
int __elk_enter(context_t **to);

/** Set up a new context.
 * @param savearea Where to put the finished stack pointer.
 * @param entry The context entry point (0 if return to caller).
 * @param mode The context execution mode.
 * @param arg1 The first argument to the entry point.
 * @param arg2 The second argument to the entry point.
 * @return 1 to indicate non-clone, else arg1.
 * This function is implemented in crt1.S.
 */
int __elk_new_context(context_t **savearea, void (entry)(void),
                      int mode, long arg);

typedef struct thread_queue {
    thread_t *head;
    thread_t *tail;
} ThreadQueue;

#define IDLE_STACK 4096                         // The idle thread stack size.
static thread_t idle_thread[PROCESSORS];        // The idle threads.
static char *idle_stack[PROCESSORS][IDLE_STACK];

static void schedule_nolock(thread_t *list);

/* The tid_mutex protects the thread id pool.
 */
static pthread_mutex_t tid_mutex = PTHREAD_MUTEX_INITIALIZER;
static int tids[THREADS];
static int *tid_in;
static int *tid_out;

/** The thread id to thread mapping.
 */
static thread_t *threads[THREADS];

/** Initialize the tid pool.
 */
static void tid_initialize(void)
{
  int i = 0;
  for (int tid = 0; tid < THREADS; ++tid) {
    if (tid == 1) {
      // Reserve tid 1 for a future spawn of init.
      continue;
    }

    tids[i++] = tid;
  }

  tid_in = &tids[THREADS];
  tid_out = &tids[0];
}

/** Allocate a thread id to a new thread.
 */
static int alloc_tid(thread_t *tp)
{
  pthread_mutex_lock(&tid_mutex);
  if (tid_out == tid_in) {
    // No more thread ids are available.
    pthread_mutex_unlock(&tid_mutex);
    return -EAGAIN;
  }
  tp->tid = *tid_out++;
  if (tid_out == &tids[THREADS]) {
    tid_out = tids;
  }
  pthread_mutex_unlock(&tid_mutex);
  threads[tp->tid] = tp;

  return tp->tid;
}

/** Deallocate a thread id.
 */
static void release_tid(int tid)
{
  if (tid < 0 || tid >= THREADS) {
    // This should never happen.
    return;
  }

  pthread_mutex_lock(&tid_mutex);
  if (tid_in == &tids[THREADS]) {
    tid_in = tids;
  } else {
    ++tid_in;
  }
  if (tid_in == tid_out) {
    // The tid pool is full.
    // This should never happen.
    pthread_mutex_unlock(&tid_mutex);
    return;
  }

  *tid_in = tid;
  // Clear the thread map entry.
  threads[tid] = NULL;
  pthread_mutex_unlock(&tid_mutex);
}

/** The idle thread.
 */
static void idle(void)
{
  for ( ;; ) {
    // Do stuff, but nothing that will block.
    // ARM should do WFI here.
  }
}

/** Create an idle thread for each processor.
 * These threads can never exit.
 */
static void create_idle_threads(void)
{
  for (int i = 0; i < PROCESSORS; ++i) {
    idle_thread[i].saved_ctx = (context_t *)&idle_stack[i][IDLE_STACK];
    idle_thread[i].priority = PRIORITIES;   // The lowest priority.
    idle_thread[i].state = IDLE;
    alloc_tid(&idle_thread[i]);
    char name[20];
    snprintf(name, 20, "idle%d", i);
    idle_thread[i].name = strdup(name);
    __elk_new_context(&idle_thread[i].saved_ctx, idle, INITIAL_PSR, 0);
  }
}

/* The ready_lock protects the following variables.
 */
static lock_t ready_lock;

static int priority;                    // The current highest priority.

#if PRIORITIES > 1 && PROCESSORS > 1
// Multiple priorities and processors.
static int processor() { return 0; }    // RICH: For now.
static thread_t *current[PROCESSORS];
static void *slice_tmo[PROCESSORS];     // Time slice timeout ID.
static ThreadQueue ready[PRIORITIES];   // The ready to run list.
static long irq_state[PROCESSORS];      // Set if an IRQ is active.

#define current current[processor()]
#define idle_thread idle_thread[processor()]
#define slice_tmo slice_tmo[processor()]
#define ready_head(pri) ready[pri].head
#define ready_tail(pri) ready[pri].tail
#define irq_state irq_state[processor()]

#elif PROCESSORS > 1
// Multiple processors, one priority.
static int processor() { return 0; }    // RICH: For now.
static thread_t *current[PROCESSORS];
static void *slice_tmo[PROCESSORS];     // Time slice timeout ID.
static ThreadQueue ready;               // The ready to run list.
static long irq_state[PROCESSORS];      // Set if an IRQ is active.

#define current current[processor()]
#define idle_thread idle_thread[processor()]
#define slice_tmo slice_tmo[processor()]
#define ready_head(pri) ready.head
#define ready_tail(pri) ready.tail
#define irq_state irq_state[processor()]

#elif PRIORITIES > 1
// One processor, multiple priorities.
static thread_t *current;
static void *slice_tmo;                 // Time slice timeout ID.
static ThreadQueue ready[PRIORITIES];
static long irq_state;                  // Set if an IRQ is active.

#define processor() 0
#define current current
#define idle_thread idle_thread[0]
#define slice_tmo slice_tmo
#define ready_head(pri) ready[pri].head
#define ready_tail(pri) ready[pri].tail

#else
// One processor, one priority.
static thread_t *current;
static void *slice_tmo;                 // Time slice timeout ID.
static ThreadQueue ready;               // The ready to run list.
static long irq_state;                  // Set if an IRQ is active.

#define processor() 0
#define current current
#define idle_thread idle_thread[0]
#define slice_tmo slice_tmo
#define ready_head(pri) ready.head
#define ready_tail(pri) ready.tail

#endif

static long long slice_time = 5000000;  // The time slice period (ns).

/**** End of ready lock protected variables. ****/

// The main thread's initial values.
static thread_t main_thread = {
  .name = "kernel",
  .priority = DEFAULT_PRIORITY,
  .state = RUNNING,
#if HAVE_CAPABILITY
  .cap = SUPERUSER_CAPABILITIES,
  .ecap = SUPERUSER_CAPABILITIES,
  .icap = SUPERUSER_CAPABILITIES,
#endif
};

/** Check the current thread's capability.
 */
int capable(capability_t cap)
{
  return CAPABLE(current, cap);
}

/** Get the current thread id.
 */
int gettid(void)
{
  return current->tid;
}

/** Get the current process id.
 */
int getpid(void)
{
  return current->pid;
}

/** Is a pid valid?
 */
int pid_valid(pid_t pid)
{
  if (pid < 0 || pid >= THREADS || threads[pid] == NULL) {
    return 0;
  }

  return 1;
}

#if HAVE_VM
/** Get the memory map of a process.
 */
vm_map_t getmap(pid_t pid)
{
  ASSERT(threads[pid] != NULL);
  return threads[pid]->map;
}

/** Get the memory map of the current process.
 */
vm_map_t getcurmap()
{
  return current->map;
}
#endif

/** Enter the IRQ state.
 * This function is called from crt1.S.
 */
void *__elk_enter_irq(void)
{
  lock_aquire(&ready_lock);
  long state = irq_state++;
  if (state) return NULL;             // Already in IRQ state.
  return current;                     // To save context.
}

/** Unlock the ready queue.
 * This function is called from crt1.S.
 */
void __elk_unlock_ready(void)
{
    lock_release(&ready_lock);
}

/** Leave the IRQ state.
 * This function is called from crt1.S.
 */
void *__elk_leave_irq(void)
{
  lock_aquire(&ready_lock);
  long state = --irq_state;
  if (state) return NULL;               // Still in IRQ state.
  return current;                       // Next context.
}

/** Get the current thread pointer.
 * This function is called from crt1.S.
 */
thread_t *__elk_thread_self()
{
  if (irq_state) return NULL;           // Nothing current if in an interrupt
                                        // context.
  return current;
}

/* Insert a thread in the ready queue.
 */
static inline void insert_thread(thread_t *tp)
{
  // A simple FIFO insertion.
  if (ready_tail(tp->priority)) {
    ready_tail(tp->priority)->next = tp;
  } else {
    ready_head(tp->priority) = tp;
  }
  ready_tail(tp->priority) = tp;
  tp->next = NULL;
  tp->state = READY;
  if (priority > tp->priority) {
    // A higher priority is ready.
    priority = tp->priority;
  }
}

/** The callback for time slice expiration.
 * @param arg The thread being timed.
 */
static void slice_callback(void *arg1, void *arg2)
{
  lock_aquire(&ready_lock);
  if ((thread_t *)arg1 == current) {
    schedule_nolock((thread_t *)arg1);
    return;
  }
  lock_release(&ready_lock);
}

/** Make the head of the ready list the running thread.
 * The ready lock must be aquired before this call.
 */
static void get_running(void)
{
  int timeslice = 1;  // Assume a time slice is needed.

  // Find the highest priority thread to run.
  current = priority < PRIORITIES ? ready_head(priority) : NULL;

#if PRIORITIES > 1
  // Check for lower priority things to run.
  while (current == NULL && priority < PRIORITIES) {
    ++priority;
    current = ready_head(priority);
  }
#endif

  if (current == NULL) {
    timeslice = 0;          // No need to timeslice for the idle thread.
    current = &idle_thread;
    priority = PRIORITIES;
  } else {
    idle_thread.state = IDLE;
    // Remove the head of the ready list.
    ready_head(priority) = ready_head(priority)->next;
    if (ready_head(priority) == NULL) {
      ready_tail(priority) = NULL;
      // No time slicing needed.
      timeslice = 0;
    }
  }

  if (slice_tmo) {
    timer_cancel_wake_at(slice_tmo);    // Cancel any existing.
    slice_tmo = NULL;
  }
  if (timeslice) {
    // Someone is waiting in the ready queue. Lets be fair.
    long long when = timer_get_monotonic(); // Get the current time.
    when += slice_time;                     // Add the slice time.
    slice_tmo = timer_wake_at(when, slice_callback, (void *)current, 0, 0);
  }

  current->state = RUNNING;
  current->next = NULL;
}

/* Schedule a list of threads.
 * The ready lock has been aquired.
 */
static void schedule_nolock(thread_t *list)
{
  thread_t *next;

  // Insert the thread list and the current thread in the ready list.
  int sched_current = 0;
  if (list != current && current != &idle_thread) {
    sched_current = 1;
  }

  while (list) {
    next = list->next;
    insert_thread(list);
    list = next;
  }

  if (sched_current) {
    // Insert the current thread in the ready list.
    insert_thread(current);
  }

  if (irq_state) {
    // The curent thread continues.
    get_running();
    lock_release(&ready_lock);
    return;
  }

  // Switch to the new thread.
  thread_t *me = current;
  get_running();
  __elk_switch(&current->saved_ctx, &me->saved_ctx);
}

/* Schedule a list of threads.
 */
static void schedule(thread_t *list)
{
  lock_aquire(&ready_lock);
  schedule_nolock(list);
}

/** Delete a thread.
 * The thread has been removed from all lists.
 */
static void thread_delete(thread_t *tp)
{
  release_tid(tp->tid);

#if ENABLEFDS
  fdset_release(&tp->fdset);
  file_t fp = tp->cwdfp;
  if (fp) {
    vfs_close(fp);
  }
#endif

  kmem_free(tp);
}

/** Change the current thread's state to
 * something besides READY or RUNNING.
 * The ready list must be locked on entry.
 */
static int nolock_change_state(int arg, state new_state)
{
  if (current->state == IDLE) {
    // Idle threads can't change state.
    return -EINVAL;
  }

  if (new_state == EXITING) {
    thread_delete(current);
    get_running();
    return __elk_enter(&current->saved_ctx);
  }

  thread_t *me = current;
  me->state = new_state;
  get_running();
  return __elk_switch_arg(arg, &current->saved_ctx, &me->saved_ctx);
}

/** Change the current thread's state to
 * something besides READY or RUNNING.
 */
static int change_state(int arg, state new_state)
{
  lock_aquire(&ready_lock);
  return nolock_change_state(arg, new_state);
}

/* Give up the remaining time slice.
 */
static int sys_sched_yield(void)
{
  schedule(current);
  return 0;
}

/** Send a message to a message queue.
 */
int send_message_q(struct queue *queue, Message msg)
{
  if (queue == NULL) {
    queue = &current->queue;
  }
  Envelope *envelope = (Envelope *)kmem_alloc(sizeof(Envelope));
  if (!envelope) {
    return ENOMEM;
  }
  envelope->message = msg;

  thread_t *wakeup = NULL;
  envelope->next = NULL;
  lock_aquire(&queue->lock);
  // Queue a envelope.
  if (queue->head) {
    queue->tail->next = envelope;
  } else {
    queue->head = envelope;
  }
  queue->tail = envelope;
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
  return 0;
}

/** Send a message to a thread.
 */
int send_message(int tid, Message msg)
{
  if (tid < 0 || tid >= THREADS) {
    return EINVAL;
  }
  thread_t *tp = threads[tid];

  if (tp == NULL) {
    return ESRCH;
  }

  return send_message_q(&tp->queue, msg);
}

Message get_message(struct queue *queue)
{
  if (queue == NULL) {
    queue = &current->queue;
  }

  Envelope *envelope = NULL;;
  do {
    lock_aquire(&queue->lock);
    // Check for queued items.
    if (queue->head) {
      envelope = queue->head;
      queue->head = envelope->next;
      if (!queue->head) {
        queue->tail = NULL;
      }
    }
    if (!envelope) {
      // Sleep until something becomes available.
      // Remove me from the ready list.
      lock_aquire(&ready_lock);
      thread_t *me = current;
      // Add me to the waiter list.
      me->next = queue->waiter;
      queue->waiter = me;
      lock_release(&queue->lock);
      int s = nolock_change_state(0, MSGWAIT);
      if (s != 0) {
        if (s < 0) {
          // An error (like EINTR) has occured.
          errno = -s;
        } else {
          // Another system event has occured, handle it.
        }

        s = -1;
        return (Message){};
      }
    } else {
      lock_release(&queue->lock);
    }
  } while(envelope == NULL);

  Message msg = envelope->message;
  kmem_free(envelope);
  return msg;
}

Message get_message_nowait(struct queue *queue)
{
  if (queue == NULL) {
    queue = &current->queue;
  }

  Envelope *envelope = NULL;
  lock_aquire(&queue->lock);
  // Check for queued items.
  if (queue->head) {
    envelope = queue->head;
    queue->head = envelope->next;
    if (!queue->head) {
      queue->tail = NULL;
    }
  }
  lock_release(&queue->lock);
  Message msg;
  if (envelope) {
    msg = envelope->message;
    kmem_free(envelope);
  } else {
    // No messages available.
    msg  = (Message){ MSG_NONE };
  }
  return msg;
}

/* Set pointer to thread ID.
 * @param tidptr Where to put the thread ID.
 * @return The tid of the calling process.
 *
 * Stub for now.
 */
static long sys_set_tid_address(int *tidptr)
{
  current->clear_child_tid = tidptr;
  return current->tid;
}

/** The timeout list.
 * This list is kept in order of expire times.
 */

struct timeout {
  struct timeout *next;
  long long when;               // When the timeout will expire.
  thread_t *waiter;             // The waiting thread...
  TimerCallback callback;       // ... or the callback function.
  void *arg1;                   // The callback function arguments.
  void *arg2;
};

static lock_t timeout_lock;
static struct timeout *timeouts;

/** Make an entry in the sleeping list and sleep
 * or schedule a callback.
 * RICH: Mark realtime timers.
 */
void *timer_wake_at(long long when,
                    TimerCallback callback, void *arg1, void *arg2, int retval)
{
  struct timeout *tmo = kmem_alloc(sizeof(struct timeout));
  if (tmo == NULL) {
    return NULL;
  }
  tmo->next = NULL;
  tmo->when = when;
  tmo->callback = callback;
  tmo->arg1 = arg1;
  tmo->arg2 = arg2;
  if (callback == NULL) {
    // Put myself to sleep and wake me when it's over.
    tmo->waiter = current;
  } else {
    tmo->waiter = NULL;
  }

  lock_aquire(&timeout_lock);
  // Search the list.
  if (timeouts == NULL) {
    timeouts = tmo;
  } else {
    struct timeout *p, *q;
    for (p = timeouts, q = NULL; p && p->when <= when; q = p, p = p->next)
      continue;
    if (p) {
      // Insert befor p.
      tmo->next = p;
    }
    if (q) {
      // Insert after q.
      q->next = tmo;
    } else {
      // Insert at the head.
      timeouts = tmo;
    }
  }

  // Set up the timeout.
  timer_start(timeouts->when);
  lock_release(&timeout_lock);
  if (tmo->callback == NULL) {
    // Put myself to sleep.
    int s = change_state(retval, SLEEPING);
    if (s != 0) {
      if (s < 0) {
        // An error (like EINTR) has occured.
        errno = -s;
      } else {
        // Another system event has occured, handle it.
      }
      tmo  = NULL;
    }
  }
  return tmo;         // Return the timeout identifier (opaque).
}

/** Cancel a previously scheduled wakeup.
 * This function will cancel a previously scheduled wakeup.
 * If the wakeup caused the caller to sleep, it will be rescheduled.
 * @param id The timer id.
 * @return 0 if cancelled, else the timer has probably already expired.
 */
static int timer_cancel_wake_at(void *id)
{
  if (id == NULL) return 0;           // Nothing pending.
  int s = 0;
  lock_aquire(&timeout_lock);
  struct timeout *p, *q;
  for (p = timeouts, q = NULL; p; q = p, p = p->next) {
    if (p == id) {
      break;
    }
  }

  if (p) {
    // Found it.
    if (q) {
      q->next = p->next;
    } else {
      timeouts = p->next;
    }

    if (p->waiter) {
      // Wake up the sleeping thread.
      p->waiter->next = NULL;
      schedule(p->waiter);
    }

    kmem_free(p);
  } else {
    s = -1;                 // Not found.
  }

  lock_release(&timeout_lock);
  return s;
}

/** Cancel some previously scheduled wakeups.
 * This function will cancel previously scheduled wakeups that have the given
 * arguments.
 * @param id The timer id.
 * @param arg1 The first argument to match.
 * @param arg2 The second argument to match.
 * @return The number of threads that have been awoken.
 */
static int timer_cancel_wake_count(unsigned count, void *arg1, void *arg2,
                                   int retval)
{
  int s = 0;
  lock_aquire(&timeout_lock);
  struct timeout *p, *q;
  for (p = timeouts, q = NULL; p; q = p, p = p->next) {
    if (p->arg1 == arg1 && p->arg2 == arg2) {
      // Have a match.
      if (q) {
        q->next = p->next;
      } else {
        timeouts = p->next;
      }

      if (p->waiter) {
        // Wake up the sleeping thread.
        p->waiter->next = NULL;
        context_set_return(p->waiter->saved_ctx, retval);
        schedule(p->waiter);
        ++s;
      }

      kmem_free(p);
      if (s >= count) {
        break;
      }
    }
  }

  lock_release(&timeout_lock);
  return s;
}

/** Timer expired handler.
 * This function can be called in an interrupt context.
 */
long long timer_expired(long long when)
{
  lock_aquire(&timeout_lock);
  thread_t *ready = NULL;
  thread_t *next = NULL;
  while (timeouts && timeouts->when <= when) {
    struct timeout *tmo = timeouts;
    timeouts = timeouts->next;
    // If the timeout hasn't been cancelled.
    if (tmo->waiter) {
      // Make sure the earliest are scheduled first.
      if (next) {
        next->next = tmo->waiter;
        next = next->next;
      } else {
        ready = tmo->waiter;
        next = ready;
      }
      next->next = NULL;
    }

    if (tmo->callback) {
      // Call the callback function.
      tmo->callback(tmo->arg1, tmo->arg2);
    }

    kmem_free(tmo);
  }

  if (timeouts == NULL) {
    when = 0;
  } else {
    when = timeouts->when;
  }

  lock_release(&timeout_lock);

  if (ready) {
    // Schedule the ready threads.
    schedule(ready);
  }

  return when;        // Schedule the next timeout, if any.
}

#if !HAVE_VM
// RICH: temporary definitions.
int vm_allocate(pid_t tid, void **addr, size_t size, int anywhere)
{
  if (!anywhere) return 1;      // Fail.
  *addr = calloc(size, 1);
  return *addr == NULL;
}

int vm_free(pid_t tid, void *addr)
{
  kmem_free(addr);
  return 0;
}

void *kmem_alloc(size_t size)
{
  return malloc(size);
}

void kmem_free(void *p)
{
  free(p);
}

paddr_t page_alloc(psize_t size)
{
  return (uintptr_t)malloc(size);
}

void page_free(paddr_t p, psize_t size)
{
  free((void *)p);
}

#endif

static long sys_clone(unsigned long flags, void *stack, int *ptid,
#if defined(__arm__) || defined(__microblaze__) || defined(__ppc__) || \
    defined(__mips__)
                      void *regs, int *ctid,
#elif defined(__i386__) || defined(__x86_64__)
                      int *ctid, void *regs,
#else
  #error clone arguments not defined
#endif
                      long arg5, long data, void (*entry)(void))
{
  // Create a new thread, copying context.
  thread_t *tp = kmem_alloc(sizeof(thread_t)); 
  if (!tp) {
    return -ENOMEM;
  }

  // Inherit the parent's attributes.
  *tp = *current;
  tp->clear_child_tid = NULL;

  // Mark the parent.
  tp->ppid = tp->tid;

  int tid = alloc_tid(tp);
  if (tid < 0) {
    kmem_free(tp);
    return -EAGAIN;
  }

#if HAVE_VM
  if (flags & CLONE_VM) {
    // Share the address space of the parent.
    vm_reference(tp->map);
  } else {
    // This thread will have a new address space.
    tp->map = vm_dup(tp->map);
    if (tp->map == NULL) {
      // The dup failed.
      release_tid(tid);
      kmem_free(tp);
      return -ENOMEM;
    }
  }
#endif

#if ENABLEFDS
  // Clone or copy the file descriptor set.
  int s = fdset_clone(&tp->fdset, flags & CLONE_FILES);
  if (s < 0) {
    release_tid(tid);
    kmem_free(tp);
    return s;
  }

  // Increment the current directory reference counts.
  file_t fp = tp->cwdfp;
  if (fp) {
    vref(fp->f_vnode);
    ++fp->f_count;
  }

#endif

  // Record the TLS.
  tp->tls = regs;

  if (flags & CLONE_CHILD_CLEARTID) {
    VALIDATE_ADDRESS(ctid, sizeof(*ctid), VALID_WR);
    tp->clear_child_tid = ctid;
  }

  if (flags & CLONE_CHILD_SETTID) {
    VALIDATE_ADDRESS(ctid, sizeof(*ctid), VALID_WR);
    // RICH: This write should be in the child's address space.
    if (ctid) {
      *ctid = tp->tid;
    }
  }

  if (flags & CLONE_PARENT_SETTID) {
    VALIDATE_ADDRESS(ptid, sizeof(*ptid), VALID_WR);
    // RICH: This write should also be in the child's address space.
    *ptid = tp->tid;
  }

  tp->next = NULL;
  if (priority == 0) {
    priority = DEFAULT_PRIORITY;
  } else if (priority >= PRIORITIES) {
    priority = PRIORITIES - 1;
  }
  tp->priority = priority;

  tp->queue = (struct queue)MSG_QUEUE_INITIALIZER;

  context_t *cp = (context_t *)stack;
  tp->saved_ctx = cp;
  // Copy registers.
  *(cp - 1) = *current->saved_ctx;

  __elk_new_context(&tp->saved_ctx, entry, INITIAL_PSR, 0);

  // Schedule the thread.
  schedule(tp);
  return tp->tid;
}

#define FUTEX_MAGIC 0xbeadcafe

/* Manipulate a futex.
 */
static int sys_futex(int *uaddr, int op, int val,
                     const struct timespec *timeout, int *uaddr2, int val3)
{
  int s = 0;

  switch (op & 0x7F) {
  default:
  case FUTEX_FD:
  case FUTEX_REQUEUE:
  case FUTEX_CMP_REQUEUE:
  case FUTEX_WAKE_OP:
  case FUTEX_LOCK_PI:
  case FUTEX_UNLOCK_PI:
  case FUTEX_TRYLOCK_PI:
  case FUTEX_WAIT_BITSET:
    printf("unhandled futex operation: %d\n", op);
    return -ENOSYS;

  case FUTEX_WAIT: {
    long long when;
    if (timeout == NULL) {
      // RICH: Forever.
      when = 0x7FFFFFFFFFFFLL;
    } else {
      when = timeout->tv_sec * 1000000000LL + timeout->tv_nsec;
      // Turn a relative time in to an absolute time.
      when += timer_get_monotonic();
    }

    if (timer_wake_at(when, NULL, (void *)FUTEX_MAGIC, uaddr,
                            -ETIMEDOUT) == NULL) {
      s = -ENOMEM;
    }
    break;
  }

  case FUTEX_WAKE:
    s = timer_cancel_wake_count(val, (void *)FUTEX_MAGIC, uaddr, 0);
    break;
  }

  return s;
}

/* Get the robust futex list.
 */
static int sys_get_robust_list(int tid, struct robust_list_head **head,
                               size_t len)
{
  if (tid == 0) {
    tid = current->tid;
  }

  if (len != sizeof(struct robust_futex_list *)) {
    return -EINVAL;
  }

  if (tid == current->tid || current->pid == getpid()) {
    // Short cut checks for the current thread and process.
    if (copyout(&current->robust_list, head,
                sizeof(struct robust_futex_list *)))
      return -EFAULT;
    return 0;
  }

  // The request is being made by another process.
  if (tid < 0 || tid >= THREADS) {
    return -EINVAL;
  }

  thread_t *tp = threads[tid];
  if (tp == NULL) {
    // Invalid thread id.
    return -ESRCH;
  }

  if (!CAPABLE(current, CAP_SYS_PTRACE)) {
    return -EPERM;
  }

  if (copyout(&tp->robust_list, head, sizeof(struct robust_list_head *)))
    return -EFAULT;
  return 0;
}

/* Set the robust futex list.
 */
static int sys_set_robust_list(struct robust_list_head *head, size_t len)
{
  if (len != sizeof(struct robust_list_head))
    return -EINVAL;

  current->robust_list = head;
  return 0;
}

/* Send a signal to a thread.
 */
static int sys_tkill(int tid, int sig)
{
  if (tid < 0 || tid >= THREADS) {
    return -EINVAL;
  }

  thread_t *tp = threads[tid];
  if (tp == NULL) {
    // Invalid thread id.
    return -ESRCH;
  }

  lock_aquire(&ready_lock);
  if (tp == current) {
    // The currently running thread is being signaled.
  } else if (tp->state == READY) {
    // Remove this thread from its ready list.
    thread_t *p, *l;
    for (p = ready_head(tp->priority), l = NULL;
         p != tp; l = p, p = p->next) {
      continue;
    }

    if (p == NULL) {
      // The thread wasn't found. This shouldn't happen.
      lock_release(&ready_lock);
      return -ESRCH;
    }

    // Remove thread from the ready list.
    if (l) {
      l->next = p->next;
    } else {
      ready_head(tp->priority) = p->next;
    }
  } else {
    printf("RICH: need to handle non-current, non-ready threads\n");
  }

  // Get the next runnable thread.
  // RICH: get_running();
  lock_release(&ready_lock);
  return 0;
}

/** Send a signal to a process.
 */
int signal_post(pid_t pid, int sig)
{
  // RICH: Send a signal to a process.
  return 0;
}
/** Schedule a defered procedure call.
 */
void sched_dpc(struct dpc *dpc, void (*fn)(void *), void *arg)
{
  // RICH: Schedule a delayed procedure call.
}

static void sys_exit(int status)
{
  if (current->clear_child_tid) {
    *current->clear_child_tid = 0;
    timer_cancel_wake_count(~0, (void *)FUTEX_MAGIC, current->clear_child_tid,
                            0);
  }

  change_state(0, EXITING);
}

static int sys_exit_group(int status)
{
  return -ENOSYS;
}

static gid_t sys_getegid(void)
{
  return current->egid;
}

static uid_t sys_geteuid(void)
{
  return current->euid;
}

static gid_t sys_getgid(void)
{
  return current->gid;
}

static pid_t sys_getpgid(pid_t pid)
{
  if (pid == 0) {
    return current->pgid;
  }

  if (pid < 0 || pid >= THREADS || threads[pid] == NULL) {
    return -ESRCH;
  }

  return threads[pid]->pgid;
}

static pid_t sys_getpgrp(void)
{
  return current->pgid;
}

static pid_t sys_getpid(void)
{
  return current->pid;
}

static pid_t sys_getppid(void)
{
  return current->ppid;
}

static pid_t sys_getsid(void)
{
  return current->sid;
}

static pid_t sys_gettid(void)
{
  return current->tid;
}

static uid_t sys_getuid(void)
{
  return current->uid;
}

static int sys_setpgid(pid_t pid, pid_t pgid)
{
  // Check for a valid pgid.
  if (pgid < 0 || pgid >= THREADS) {
    return -EINVAL;
  }

  // Check for a valid pid.
  if (pid < 0 || pid >= THREADS) {
    return -EINVAL;
  }

  if (pid == 0) {
    // Use the caller's pid.
    pid = current->tid;
  }

  thread_t *pid_thread = threads[pid];
  if (pid_thread == NULL) {
    // Not an active pid.
    return -ESRCH;
  }

  if (pid != current->tid && current->tid != pid_thread->ppid) {
    // Not the current process or a child of the current process.
    return -ESRCH;
  }

  if (pgid == 0) {
    // Use the pid as the pgid.
    pgid = pid_thread->tid;
  }

  thread_t *pgid_thread = threads[pgid];
  if (pgid_thread == NULL) {
    // Not an active id.
    return -ESRCH;
  }

  if (pid_thread->sid != pgid_thread->sid) {
    // Can't move to a different session.
    return -EPERM;
  }

  if (pgid_thread->pgid == pgid_thread->tid) {
    // Can't change the pgid of a process group leader.
    return -EPERM;
  }

#if RICH
  if (pgid_thread-><thread has executed execve>) {
    return -EACCES;
  }
#endif

  pgid_thread->pgid = pid;
  return 0;
}

static int sys_setresgid(gid_t rgid, gid_t egid, gid_t sgid)
{
  if (rgid != -1) {
    // Changing the gid.
    if (!CAPABLE(current, CAP_SETGID) &&
        rgid != current->gid &&
        rgid != current->egid &&
        rgid != current->sgid) {
      // An Unprivileged user can only set the gid to its gid, egid, or sgid.
      return -EPERM;
    }
  }

  if (egid != -1) {
    // Changing the egid.
    if (!CAPABLE(current, CAP_SETGID) &&
        egid != current->gid &&
        egid != current->egid &&
        egid != current->sgid) {
      // An Unprivileged user can only set the egid to its gid, egid, or sgid.
      return -EPERM;
    }
  }

  if (sgid != -1) {
    // Changing the egid.
    if (!CAPABLE(current, CAP_SETGID) &&
        sgid != current->gid &&
        sgid != current->egid &&
        sgid != current->sgid) {
      // An Unprivileged user can only set the sgid to its gid, egid, or sgid.
      return -EPERM;
    }
  }

  if (rgid != -1) {
    current->gid = rgid;
  }

  if (egid != -1) {
    current->egid = egid;
    current->fgid = egid;
  }

  if (sgid != -1) {
    current->sgid = sgid;
  }
  return 0;
}

static int sys_setregid(gid_t rgid, gid_t egid)
{
  return sys_setresgid(rgid, egid, -1);
}

static int sys_setgid(gid_t gid)
{
  if (!CAPABLE(current, CAP_SETGID)) {
    // Set only the effective gid.
    return sys_setresgid(-1, gid, -1);
  }

  // Set all the gids.
  return sys_setresgid(gid, gid, gid);
}

static int sys_setresuid(uid_t ruid, uid_t euid, uid_t suid)
{
#if HAVE_CAPABILITY
  int rzero = current->uid == 0;
  int ezero = current->euid = 0;
  int szero = current->euid = 0;
#endif
  if (ruid != -1) {
    // Changing the uid.
    if (!CAPABLE(current, CAP_SETUID) &&
        ruid != current->uid &&
        ruid != current->euid &&
        ruid != current->suid) {
      // An Unprivileged user can only set the uid to its uid, euid, or suid.
      return -EPERM;
    }
  }

  if (euid != -1) {
    // Changing the euid.
    if (!CAPABLE(current, CAP_SETUID) &&
        euid != current->uid &&
        euid != current->euid &&
        euid != current->suid) {
      // An Unprivileged user can only set the euid to its uid, euid, or suid.
      return -EPERM;
    }
  }

  if (suid != -1) {
    // Changing the suid.
    if (!CAPABLE(current, CAP_SETUID) &&
        suid != current->uid &&
        suid != current->euid &&
        suid != current->suid) {
      // An Unprivileged user can only set the suid to its uid, euid, or suid.
      return -EPERM;
    }
  }

  if (ruid != -1) {
    current->uid = ruid;
  }

  if (euid != -1) {
    current->euid = euid;
    current->fuid = euid;
  }

  if (suid != -1) {
    current->suid = suid;
  }

#if HAVE_CAPABILITY
  if (rzero || ezero || szero) {
    // One or both started as zero.
    if (current->uid && current->euid && current->suid) {
      // All are non-zero. Nor more capabilities.
      current->cap = NO_CAPABILITIES;
      current->ecap = NO_CAPABILITIES;
    } else if (current->euid) {
      // No more effective capabilities.
      current->ecap = NO_CAPABILITIES;
    }
  } else if (!ezero && current->euid == 0) {
    // The euid changed from non-zero to zero: restore capabilities.
    current->ecap = current->cap;
  }
#endif

  return 0;
}

static int sys_setreuid(uid_t ruid, uid_t euid)
{
  return sys_setresuid(ruid, euid, -1);
}

static int sys_setuid(uid_t uid)
{
  if (!CAPABLE(current, CAP_SETGID)) {
    // Set only the effective uid.
    return sys_setresuid(-1, uid, -1);
  }

  // Set all the uids.
  return sys_setresuid(uid, uid, uid);
}

static int sys_setsid(void)
{
  if (current->tid == current->pgid) {
    // This is currently a process group leader.
    return -EPERM;
  }

  current->pgid = current->tid;
  return current->tid;
}

static mode_t sys_umask(mode_t new)
{
  mode_t old = current->umask;
  current->umask = new;
  return old;
}

#if ENABLEFDS
/** Get a file descriptor.
 */
int allocfd(file_t fp)
{
  return fdset_add(current->fdset, fp);
}

/** Get a file pointer corresponding to a file descriptor.
 */
int getfile(int fd, file_t *filep)
{
  return fdset_getfile(current->fdset, fd, filep);
}

/** Create a new file descriptor or replace the file
 * pointer of a current file descriptor.
 * If free, find the first available free file descriptor.
 */
int getdup(int fd, file_t *filep, int free)
{
  return fdset_getdup(current->fdset, fd, filep, free);
}

/** Set a file pointer corresponding to a file descriptor.
 */
int setfile(int fd, file_t file)
{
  return fdset_setfile(current->fdset, fd, file);
}

/** Replace the old cwd fp with a new one.
 */
file_t replacecwd(file_t fp)
{
  file_t oldfp = current->cwdfp;
  current->cwdfp = fp;
  return oldfp;
}

/** Get a file path.
 * This function returns the full path name for the file name.
 */
int getpath(const char *name, char *path)
{
  // Find the current directory name.
  const char *cwd = current->cwdfp ? current->cwdfp->f_vnode->v_path : "/";
  const char *src = name;
  char *tgt = path;
  int len = 0;
  if (src[0] == '/') {
    // The path starts at the root.
    *tgt++ = *src++;
    ++len;
  } else {
    // The current working directory starts the path.
    strlcpy(tgt, cwd, PATH_MAX);
    len = strlen(cwd);
    tgt += len;
    // If cwd is not the root directory and the name doesn't start with "."
    // add a trailing "/" to the current directory.
    if (len > 1 && *src != '\0' && *src != '.') {
      if (++len >= PATH_MAX)
        return -ENAMETOOLONG;
      *tgt++ = '/';
    }
  }

  // Copy the src in to the target, removing ./ and ../
  while (*src) {
    // Find the end of this component.
    const char *p = src;
    while (*p != '/' && *p != '\0')
      ++p;
    int count = p - src;                // The length of the component.
    if (count == 2 && strncmp(src, "..", count) == 0) {
      if (len >= 2) {
        // Go back to the previous '/'.
        len -= 2;
        tgt -= 2;
        while (*tgt != '/') {
          --tgt;
          --len;
        }

        if (len == 0) {
          // At the initial '/'.
          ++tgt;
          ++len;
        }
      }
    } else if (count == 1 && *src == '.') {
      // Ignore '.' and './'.
    } else {
      while (src != p) {
        if (++len >= PATH_MAX)
          return -ENAMETOOLONG;
        *tgt++ = *src++;
      }
    }

    if (*p) {
      // We are at a '/'. Add a slash to the target if it isn't redundant.
      if (len > 0 && *(tgt - 1) != '/') {
        if (++len >= PATH_MAX)
          return -ENAMETOOLONG;
        *tgt++ = '/';
      }

      src = p + 1;                      // Skip the '/'.
    } else {
      src = p;
    }
  }

  if (++len >= PATH_MAX)
    return -ENAMETOOLONG;

  *tgt = '\0';

  return 0;
}

#endif // ENABLEFDS

#if THREAD_COMMANDS

static int psCommand(int argc, char **argv)
{
  if (argc <= 0) {
    printf("show process information.\n");
    return COMMAND_OK;
  }

  printf("%6.6s %6.6s %10.10s %-10.10s %5.5s %-10.10s \n",
         "PID", "TID", "TADR", "STATE", "PRI", "NAME");
  for (int i = 0;  i < THREADS; ++i) {
    thread_t *t =  threads[i];
    if (t == NULL) {
      continue;
    }
    printf("%6d ", t->pid);
    printf("%6d ", t->tid);
    printf("%8p ", t);
    printf("%-10.10s ", state_names[t->state]);
    printf("%5d ", t->priority);
    printf(t->pid == t->tid ? "%s" : "[%s]", t->name ? t->name : "");
    printf("\n");
  }

  return COMMAND_OK;
}

static int ssCommand(int argc, char **argv)
{
  if (argc <= 0) {
    printf("show sleeping thread information.\n");
    return COMMAND_OK;
  }

  lock_aquire(&timeout_lock);
  int i = 0;
  for (struct timeout *tmo = timeouts; tmo; tmo = tmo->next) {
    printf("%d %lld", i, tmo->when);
    for (thread_t *tp = tmo->waiter; tp; tp = tp->next) {
      printf(" %d", tp->tid);
    }
    printf("\n");
  }
  lock_release(&timeout_lock);

  return COMMAND_OK;
}

/** Create a section heading for the help command.
 */
static int sectionCommand(int argc, char **argv)
{
  if (argc <= 0 ) {
    printf("Thread Commands:\n");
  }
  return COMMAND_OK;
}

#endif  // THREAD_COMMANDS

#if ELK_DEBUG
int debug;

/* Set debug level.
 */
static int dbgCommand(int argc, char **argv)
{
  if (argc <= 0) {
    printf("set the debug level\n");
    return COMMAND_OK;
  }

  if (argc != 2) {
    printf("usage: dbg <value>\n");
    return COMMAND_ERROR;
  }

  debug = strtol(argv[1], 0, 0);
  return COMMAND_OK;
}
#endif

// RICH: Temporary hack.
#if HAVE_VM
extern char __end[];            // The end of the .bss area.
extern char *__heap_end__;      // The bottom of the allocated stacks.

struct bootinfo bootinfo;
#endif

/* Initialize the thread handling code.
 */
ELK_CONSTRUCTOR()
{
  /** We set up a thread_self pointer early to tell
   * the C library that we support threading.
   */
  current = &main_thread;
#if HAVE_VM
  bootinfo.nr_rams = 1;
  // RICH Save a few pages for malloc() for now.
  bootinfo.ram[0].base = round_page((uintptr_t)__end) + (4096 * 4);
  bootinfo.ram[0].size = trunc_page((uintptr_t)__heap_end__)
                         - bootinfo.ram[0].base;
  bootinfo.ram[0].type = MT_USABLE;
  page_init();
  kmem_init();
#endif

#if THREAD_COMMANDS
  command_insert(NULL, sectionCommand);
  command_insert("ps", psCommand);
  command_insert("ss", ssCommand);
#endif
#if ELK_DEBUG
  command_insert("dbg", dbgCommand);
#endif

  SYSCALL(clone);
  SYSCALL(exit);
  SYSCALL(exit_group);
  SYSCALL(futex);
  SYSCALL(getegid);
  SYSCALL(geteuid);
  SYSCALL(getgid);
  SYSCALL(getpgid);
  SYSCALL(getpgrp);
  SYSCALL(getpid);
  SYSCALL(getppid);
  SYSCALL(getsid);
  SYSCALL(gettid);
  SYSCALL(getuid);
  SYSCALL(get_robust_list);
  SYSCALL(sched_yield);
  SYSCALL(setgid);
  SYSCALL(setpgid);
  SYSCALL(setresgid);
  SYSCALL(setregid);
  SYSCALL(setresuid);
  SYSCALL(setreuid);
  SYSCALL(setsid);
  SYSCALL(setuid);
  SYSCALL(set_robust_list);
  SYSCALL(set_tid_address);
  SYSCALL(tkill);
  SYSCALL(umask);
}

C_CONSTRUCTOR()
{
  // Set up the tid pool.
  tid_initialize();

  // The main thread is what's running right now.
  alloc_tid(current);

#if HAVE_VM
  // Set up the kernel memory map.
  current->map = vm_init();
#endif

#if ENABLEFDS
  // Initialize the main thread's file descriptors.
  int s = fdset_new(&current->fdset);
  ASSERT(s == 0);
#endif

  current->pid = current->tid;          // The main thread starts a group.
  priority = current->priority;

  // Create the idle thread(s).
  create_idle_threads();
}

