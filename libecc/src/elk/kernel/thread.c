/** Schedule threads for execution.
 */
#include <syscalls.h>                   // For syscall numbers.
#include <sys/types.h>
#define _GNU_SOURCE
#include <stdio.h>
#include <sched.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <inttypes.h>

#include "timer.h"
#define DEFINE_STATE_STRINGS
#include "thread.h"
#include "file.h"
#include "semaphore.h"
#include "command.h"

// Make threads a loadable feature.
FEATURE(thread, thread)

#define THREADS 1024                    // The number of threads supported.

#define PRIORITIES 3                    // The number of priorities to support:
                                        // (0..PRIORITIES - 1). 0 is highest.
#define DEFAULT_PRIORITY ((PRIORITIES)/2)
#define PROCESSORS 1                    // The number of processors to support.

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
typedef struct thread
{
  // The saved_ctx and tls fields must be first in the thread struct.
  context_t *saved_ctx;         // The thread's saved context.
  void *tls;                    // The thread's user space storage.
  struct thread *next;          // Next thread in any list.
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
  fdset_t fdset;                // File descriptors used by the thread.
  MsgQueue queue;               // The thread's message queue.
} thread;

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
    thread *head;
    thread *tail;
} ThreadQueue;

#define IDLE_STACK 4096                 // The idle thread stack size.
static thread idle_thread[PROCESSORS];  // The idle threads.
static char *idle_stack[PROCESSORS][IDLE_STACK];

static void schedule_nolock(thread *list);

/* The tid_lock protects the thread id pool.
 */
static lock_t tid_lock;
static int tids[THREADS];
static int *tid_in;
static int *tid_out;

/** The thread id to thread mapping.
 */
static thread *threads[THREADS];

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
static int alloc_tid(thread *tp)
{
  __elk_lock_aquire(&tid_lock);
  if (tid_out == tid_in) {
    // No more thread ids are available.
    __elk_lock_release(&tid_lock);
    return -EAGAIN;
  }
  tp->tid = *tid_out++;
  if (tid_out == &tids[THREADS]) {
    tid_out = tids;
  }
  __elk_lock_release(&tid_lock);
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

  __elk_lock_aquire(&tid_lock);
  if (tid_in == &tids[THREADS]) {
    tid_in = tids;
  } else {
    ++tid_in;
  }
  if (tid_in == tid_out) {
    // The tid pool is full.
    // This should never happen.
    __elk_lock_release(&tid_lock);
    return;
  }

  *tid_in = tid;
  // Clear the thread map entry.
  threads[tid] = NULL;
  __elk_lock_release(&tid_lock);
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
static thread *current[PROCESSORS];
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
static thread *current[PROCESSORS];
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
static thread *current;
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
static thread *current;
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
static thread main_thread = {
  .name = "kernel",
  .priority = DEFAULT_PRIORITY,
  .state = RUNNING,
#if defined(HAVE_CAPABILITY)
  .cap = SUPERUSER_CAPABILITIES,
  .ecap = SUPERUSER_CAPABILITIES,
  .icap = SUPERUSER_CAPABILITIES,
#endif
};

/** Get the current thread id.
 */
int gettid(void)
{
  return current->tid;
}

/** Enter the IRQ state.
 * This function is called from crt1.S.
 */
void *__elk_enter_irq(void)
{
  __elk_lock_aquire(&ready_lock);
  long state = irq_state++;
  if (state) return 0;                // Already in IRQ state.
  return current;                     // To save context.
}

void __elk_unlock_ready(void)
{
    __elk_lock_release(&ready_lock);
}

/** Leave the IRQ state.
 * This function is called from crt1.S.
 */
void *__elk_leave_irq(void)
{
  __elk_lock_aquire(&ready_lock);
  long state = --irq_state;
  if (state) return 0;                // Still in IRQ state.
  return current;                     // Next context.
}

/** Get the current thread pointer.
 * This function is called from crt1.S.
 */
thread *__elk_thread_self()
{
  return current;
}

/* Insert a thread in the ready queue.
 */
static inline void insert_thread(thread *tp)
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
  __elk_lock_aquire(&ready_lock);
  if ((thread *)arg1 == current) {
    schedule_nolock((thread *)arg1);
    return;
  }
  __elk_lock_release(&ready_lock);
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
    long long when = __elk_timer_get_monotonic(); // Get the current time.
    when += slice_time;                     // Add the slice time.
    slice_tmo = __elk_timer_wake_at(when, slice_callback, (void *)current, 0);
  }

  current->state = RUNNING;
  current->next = NULL;
}

/* Schedule a list of threads.
 * The ready lock has been aquired.
 */
static void schedule_nolock(thread *list)
{
  thread *next;

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
    __elk_lock_release(&ready_lock);
    return;
  }

  // Switch to the new thread.
  thread *me = current;
  get_running();
  __elk_switch(&current->saved_ctx, &me->saved_ctx);
}

/* Schedule a list of threads.
 */
static void schedule(thread *list)
{
  __elk_lock_aquire(&ready_lock);
  schedule_nolock(list);
}

/** Delete a thread.
 * The thread has been removed from all lists.
 */
static void thread_delete(thread *tp)
{
  release_tid(tp->tid);
  __elk_fdset_release(&tp->fdset);
  free(tp);
}

/** Change the current thread's state to
 * something besides READY or RUNNING.
 * The ready list must be locked on entry.
 */
static int nolock_change_state(int arg, __elk_state new_state)
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

  thread *me = current;
  me->state = new_state;
  get_running();
  return __elk_switch_arg(arg, &current->saved_ctx, &me->saved_ctx);
}

/** Change the current thread's state to
 * something besides READY or RUNNING.
 */
static int change_state(int arg, __elk_state new_state)
{
  __elk_lock_aquire(&ready_lock);
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
int __elk_send_message_q(MsgQueue *queue, Message msg)
{
  if (queue == NULL) {
    queue = &current->queue;
  }
  Envelope *envelope = (Envelope *)malloc(sizeof(Envelope));
  if (!envelope) {
    return ENOMEM;
  }
  envelope->message = msg;

  thread *wakeup = NULL;
  envelope->next = NULL;
  __elk_lock_aquire(&queue->lock);
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
  __elk_lock_release(&queue->lock);
  if (wakeup) {
    // Schedule the sleeping threads.
    schedule(wakeup);
  }
  return 0;
}

/** Send a message to a thread.
 */
int __elk_send_message(int tid, Message msg)
{
  if (tid < 0 || tid >= THREADS) {
    return EINVAL;
  }
  thread *tp = threads[tid];

  if (tp == NULL) {
    return ESRCH;
  }

  return __elk_send_message_q(&tp->queue, msg);
}

Message get_message(MsgQueue *queue)
{
  if (queue == NULL) {
    queue = &current->queue;
  }

  Envelope *envelope = NULL;;
  do {
    __elk_lock_aquire(&queue->lock);
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
      __elk_lock_aquire(&ready_lock);
      thread *me = current;
      // Add me to the waiter list.
      me->next = queue->waiter;
      queue->waiter = me;
      __elk_lock_release(&queue->lock);
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
      __elk_lock_release(&queue->lock);
    }
  } while(envelope == NULL);

  Message msg = envelope->message;
  free(envelope);
  return msg;
}

Message get_message_nowait(MsgQueue *queue)
{
  if (queue == NULL) {
    queue = &current->queue;
  }

  Envelope *envelope = NULL;
  __elk_lock_aquire(&queue->lock);
  // Check for queued items.
  if (queue->head) {
    envelope = queue->head;
    queue->head = envelope->next;
    if (!queue->head) {
      queue->tail = NULL;
    }
  }
  __elk_lock_release(&queue->lock);
  Message msg;
  if (envelope) {
    msg = envelope->message;
    free(envelope);
  } else {
    // No messages available.
    msg  = (Message){ MSG_NONE };
  }
  return msg;
}

/* Set pointer to thread ID.
 * @param tidptr Where to put the thread ID.
 * @return The PID of the calling process.
 *
 * Stub for now.
 */
static long sys_set_tid_address(int *tidptr)
{
  current->clear_child_tid = tidptr;
  return current->tid;
}

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
  thread *tp = malloc(sizeof(thread));    // RICH: bin.
  if (!tp) {
    return -ENOMEM;
  }

  // Inherit the parent's attributes.
  *tp = *current;

  // Mark the parent.
  tp->ppid = tp->tid;

  int s = alloc_tid(tp);
  if (s < 0) {
    free(tp);
    return -EAGAIN;
  }

  // Clone or copy the file descriptor set.
  s = __elk_fdset_clone(&tp->fdset, flags & CLONE_FILES);
  if (s < 0) {
    free(tp);
    return s;
  }

  // Record the TLS.
  tp->tls = regs;
  if (flags & CLONE_CHILD_CLEARTID) {
    VALIDATE_ADDRESS(ctid, sizeof(*ctid), VALID_RD);
    tp->clear_child_tid = ctid;
  }

  if (flags & CLONE_CHILD_SETTID) {
    VALIDATE_ADDRESS(ctid, sizeof(*ctid), VALID_WR);
    tp->set_child_tid = ctid;
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

  tp->set_child_tid = NULL;
  tp->clear_child_tid = NULL;
  tp->queue = (MsgQueue)MSG_QUEUE_INITIALIZER;

  context_t *cp = (context_t *)stack;
  tp->saved_ctx = cp;
  // Copy registers.
  *(cp - 1) = *current->saved_ctx;

  __elk_new_context(&tp->saved_ctx, entry, INITIAL_PSR, 0);

  // Schedule the thread.
  schedule(tp);
  return tp->tid;
}

/* Manipulate a futex.
 */
static int sys_futex(int *uaddr, int op, int val,
                     const struct timespec *timeout, int *uaddr2, int val3)
{
  switch (op & 0x7F) {
  default:
  case FUTEX_WAKE:
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

  case FUTEX_WAIT:
    // RICH: Fake for now.
    *uaddr = 0;
    break;
  }

  return 0;
}

/* Send a signal to a thread.
 */
static int sys_tkill(int tid, int sig)
{
  if (tid < 0 || tid >= THREADS) {
    return -EINVAL;
  }

  thread *tp = threads[tid];
  if (tp == NULL) {
    // Invalid thread id.
    return -ESRCH;
  }

  __elk_lock_aquire(&ready_lock);
  if (tp == current) {
    // The currently running thread is being signaled.
  } else if (tp->state == READY) {
    // Remove this thread from its ready list.
    thread *p, *l;
    for (p = ready_head(tp->priority), l = NULL;
         p != tp; l = p, p = p->next) {
      continue;
    }

    if (p == NULL) {
      // The thread wasn't found. This shouldn't happen.
      __elk_lock_release(&ready_lock);
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
  __elk_lock_release(&ready_lock);
  return 0;
}

static void sys_exit(int status)
{
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

  thread *pid_thread = threads[pid];
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

  thread *pgid_thread = threads[pgid];
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
#if defined(HAVE_CAPABILITY)
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

#if defined(HAVE_CAPABILITY)
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

static int tsCommand(int argc, char **argv)
{
  if (argc <= 0) {
    printf("show thread information.\n");
    return COMMAND_OK;
  }

  printf("%6.6s %6.6s %10.10s %-10.10s %5.5s %-10.10s \n",
         "PID", "TID", "TADR", "STATE", "PRI", "NAME");
  for (int i = 0;  i < THREADS; ++i) {
    thread *t =  threads[i];
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

/** The timeout list.
 * This list is kept in order of expire times.
 */

struct timeout {
  struct timeout *next;
  long long when;               // When the timeout will expire.
  thread *waiter;               // The waiting thread...
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
void *__elk_timer_wake_at(long long when,
                    TimerCallback callback, void *arg1, void *arg2)
{
  struct timeout *tmo = malloc(sizeof(struct timeout));
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

  __elk_lock_aquire(&timeout_lock);
  // Search the list.
  if (timeouts == NULL) {
    timeouts = tmo;
  } else {
    struct timeout *p, *q;
    for (p = timeouts, q = NULL; p && p->when < when; q = p, p = p->next)
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
  __elk_timer_start(timeouts->when);
  __elk_lock_release(&timeout_lock);
  if (tmo->callback == NULL) {
    // Put myself to sleep.
    int s = change_state(0, TIMEOUT);
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
  __elk_lock_aquire(&timeout_lock);
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

    free(p);
  } else {
    s = -1;                 // Not found.
  }

  __elk_lock_release(&timeout_lock);
  return s;
}

/** Timer expired handler.
 * This function is called in an interrupt context.
 */
long long __elk_timer_expired(long long when)
{
  __elk_lock_aquire(&timeout_lock);
  thread *ready = NULL;
  thread *next = NULL;
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

    free(tmo);
  }

  if (timeouts == NULL) {
    when = 0;
  } else {
    when = timeouts->when;
  }

  __elk_lock_release(&timeout_lock);

  if (ready) {
    // Schedule the ready threads.
    schedule(ready);
  }

  return when;        // Schedule the next timeout, if any.
}

/** Initialize a semaphore.
 * @param sem A pointer to the semaphore.
 * @param pshared != 0 if this semaphore is shared among processes.
 *                     This is not implemented.
 * @param value The initial semaphore value.
 */
int __elk_sem_init(__elk_sem_t *sem, int pshared, unsigned int value)
{
  if (pshared) {
    // Not supported.
    errno = ENOSYS;
    return -1;
  }

  sem->lock = (lock_t)LOCK_INITIALIZER;
  sem->count = value;
  sem->waiters = NULL;
  return 0;
}

/** Wait on a semaphore.
 * @param sem A pointer to the semaphore.
 */
int __elk_sem_wait(__elk_sem_t *sem)
{
  int s = 0;
  __elk_lock_aquire(&sem->lock);
  for ( ;; ) {
    if (sem->count) {
      --sem->count;
      __elk_lock_release(&sem->lock);
      break;
    } else {
      thread *me = current;
      me->next = sem->waiters;
      sem->waiters = me;
      __elk_lock_release(&sem->lock);
      s = change_state(0, SEMWAIT);
      if (s != 0) {
        if (s < 0) {
          // An error (like EINTR) has occured.
          errno = -s;
        } else {
          // Another system event has occured, handle it.
        }
        s = -1;
        break;
      }
    }
  }
  return s;
}

/** Try to take a semaphore.
 * @param sem A pointer to the semaphore.
 */
int __elk_sem_try_wait(__elk_sem_t *sem)
{
  __elk_lock_aquire(&sem->lock);
  int s = 0;
  if (sem->count) {
    --sem->count;
  } else {
    // Would have to wait.
    errno = EAGAIN;
    s = -1;
  }
  __elk_lock_release(&sem->lock);
  return s;
}

/** This callback occurs when a __elk_sem_timedwait timeout expires.
 */
static void callback(void *arg1, void *arg2)
{
  __elk_sem_t *sem = (__elk_sem_t *)arg1;
  thread *tp = (thread *)arg2;
  __elk_lock_aquire(&sem->lock);
  thread *p, *q;
  for (p = sem->waiters, q = NULL; p; q = p, p = p->next) {
    if (p == tp) {
      // This thread timed out.
      if (q) {
        q->next = p->next;
      } else {
        sem->waiters = p->next;
      }
      p->next = NULL;
      __elk_context_set_return(p->saved_ctx, -ETIMEDOUT);
      schedule(p);
    }
  }
  __elk_lock_release(&sem->lock);
}

/** Wait on a semaphore with a timeout.
 * @param sem A pointer to the semaphore.
 * @param abs_timeout The timeout.
 */
int __elk_sem_timedwait(__elk_sem_t *sem, struct timespec *abs_timeout)
{
  int s = 0;
  __elk_lock_aquire(&sem->lock);
  for ( ;; ) {
    if (sem->count) {
      --sem->count;
      __elk_lock_release(&sem->lock);
      break;
    } else {
      long long when;
      when = abs_timeout->tv_sec * 1000000000LL + abs_timeout->tv_nsec;
      long long now = __elk_timer_get_realtime();
      if (now > when) {
        // Already expired.
        __elk_lock_release(&sem->lock);
        errno = ETIMEDOUT;
        s = -1;
      } else {
        thread *me = current;
        me->next = sem->waiters;
        sem->waiters = me;
        __elk_lock_release(&sem->lock);
        when -= __elk_timer_get_realtime_offset();
        void *t = __elk_timer_wake_at(when, callback,
                                (void *)sem, (void *)me);
        s = change_state(0, SEMTMO);
        timer_cancel_wake_at(t);
        if (s != 0) {
          if (s < 0) {
            // An error (like EINTR) has occured.
            errno = -s;
          } else {
            // Another system event has occured, handle it.
          }
          s = -1;
          break;
        }
      }
    }
  }
  return s;
}

/** Unlock a semaphore.
 * @param sem A pointer to the semaphore.
 */
int __elk_sem_post(__elk_sem_t *sem)
{
  int s = 0;
  thread *list = NULL;
  __elk_lock_aquire(&sem->lock);
  ++sem->count;
  if (sem->count == 0) {
    --sem->count;
    errno = EOVERFLOW;
    s = -1;
  } else {
    s = 0;
    if (sem->waiters) {
      list = sem->waiters;
      sem->waiters = NULL;
    }
  }
  __elk_lock_release(&sem->lock);
  if (list) {
    schedule(list);
  }
  return s;
}
/* Initialize the thread handling code.
 */
ELK_CONSTRUCTOR()
{
  /** We set up a thread_self pointer early to tell
   * the C library that we support threading.
   */
  current = &main_thread;

  // Add the "ts" command.
  command_insert("ts", tsCommand);

#define SYSCALL(name) __elk_set_syscall(SYS_ ## name, sys_ ## name)
  // Set up a set_tid_address system call.
  SYSCALL(set_tid_address);

  // Set up the sched_yield system call.
  SYSCALL(sched_yield);
  // Set up the clone system call.
  SYSCALL(clone);
  // Set up the futex system call.
  SYSCALL(futex);
  // Set up the tkill system call.
  SYSCALL(tkill);
  // Set up the exit system calls.
  SYSCALL(exit);
  SYSCALL(exit_group);

  // Various id system calls.
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
  SYSCALL(setgid);
  SYSCALL(setpgid);
  SYSCALL(setresgid);
  SYSCALL(setregid);
  SYSCALL(setgid);
  SYSCALL(setresuid);
  SYSCALL(setreuid);
  SYSCALL(setuid);
  SYSCALL(setsid);
}

C_CONSTRUCTOR()
{
  // Set up the tid pool.
  tid_initialize();

  // Set up the main and idle threads.

  // The main thread is what's running right now.
  alloc_tid(current);
  current->pid = current->tid;          // The main thread starts a group.
  priority = current->priority;

  // Create the idle thread(s).
  create_idle_threads();

#define RICH
#ifdef RICH
  // Create a file descriptor binding.
  static const fileops_t fileops = {
    fnullop_read, fnullop_write, fnullop_ioctl, fnullop_fcntl,
    fnullop_poll, fnullop_stat, fnullop_close
  };
  __elk_fdset_add(&current->fdset, FTYPE_MISC, &fileops);
#endif
}

