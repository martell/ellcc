/** Schedule threads for execution.
 */
#include <syscalls.h>                   // For syscall numbers.
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <sched.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <inttypes.h>
#include <signal.h>

#include "config.h"                     // Configuration parameters.
#include "timer.h"
#include "file.h"
#include "vnode.h"
#include "page.h"
#include "kmem.h"
#include "syspage.h"
#include "command.h"
#include "crt1.h"
#include "cpu.h"
#include "trap.h"
#include "thread.h"

#if CONFIG_ENABLEFDS
#include <stdarg.h>
#endif

// Make threads a loadable feature.
FEATURE(thread)

#if 0
#define THRD_ALLOC(ptr) kmem_alloc(sizeof(CONFIG_THREAD_SIZE))
#define thread_free(ptr) kmem_free(ptr)
#else
#define THRD_ALLOC(ptr) ({ int s = 0; \
  s = vm_allocate(0, (void **)&(ptr), CONFIG_THREAD_SIZE, 1); \
  s ? NULL : (ptr); })

/** This is the cleanup function called from enter_context in crt1.S
 */
static void thread_free(void *ptr)
{
  vm_free(0, ptr, CONFIG_THREAD_SIZE);
}
#endif

#if CONFIG_HAVE_CAPABILITY
// Set all capabilities for the superuser.
#define SUPERUSER_CAPABILITIES (~(capability_t)0)
#define NO_CAPABILITIES ((capability_t)0)

#define CAPABLE(thread, capability) \
  ((thread)->ecap & CAPABILITY_TO_BIT(capability))

#define DEFAULT_PRIORITY ((CONFIG_PRIORITIES)/2)
#endif

typedef struct
{
  int lock;
  int level;
} lock_t;

#define LOCK_INITIALIZER { 0, 0 }

static inline void lock_acquire(lock_t *lock)
{
  lock->level = splhigh();
// RICH:
#if !defined(__microblaze__)
  while(__atomic_test_and_set(&lock->lock, __ATOMIC_SEQ_CST))
      continue;
#endif
}

static inline void lock_release(lock_t *lock)
{
// RICH:
#if !defined(__microblaze__)
  __atomic_clear(&lock->lock, __ATOMIC_SEQ_CST);
#endif
  splx(lock->level);
}

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

#if CONFIG_ENABLEFDS
typedef struct fs
{
  pthread_mutex_t lock;
  int refcnt;                   // Reference count;
  vnode_t cwd;                  // The current working directory.
  vnode_t root;                 // The root directory.
  mode_t umask;                 // The current umask.
} *fs_t;
#endif

// Thread states.
typedef enum state {
  IDLE,                         // This is an idle thread.
  READY,                        // The thread is ready to run.
  RUNNING,                      // The thread is running.
  EXITING,                      // The thread is exiting.
  SLEEPING,                     // The thread is sleeping.

  LASTSTATE                     // To get the number of states.
} state;

static const char *state_names[LASTSTATE] =
{
  [IDLE] = "IDLE",
  [READY] = "READY",
  [RUNNING] = "RUNNING",
  [SLEEPING] = "SLEEPING",
};

typedef struct thread
{
  // The context and tls fields must be first in the thread struct.
  context_t *context;           // The thread's saved context.
  struct thread *next;          // Next thread in any list.
  state state;                  // The thread's state.
  unsigned flags;               // Flags associated with this thread.
  unsigned nesting;             // Suscall nesting level.
  int priority;                 // The thread's priority. 0 is highest.
  char *name;                   // The thread's name.
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
#if CONFIG_HAVE_CAPABILITY
  capability_t cap;             // The thread's capabilities.
  capability_t ecap;            // The thread's effective capabilities.
  capability_t icap;            // The thread's inheritable capabilities.
#endif
#if CONFIG_ENABLEFDS
  fdset_t fdset;                // File descriptors used by the thread.
  fs_t fs;                      // The current file system information.
#endif
#if CONFIG_VM
  vm_map_t map;                 // The process memory map.
#endif
#if CONFIG_SIGNALS
  sigset_t sigmask;             // The signal mask.
#endif
  // A pointer to the user space robust futex list.
  struct robust_list_head *robust_list;
  char *brk;                    // The brk pointer, valid only in process lead.
} thread_t;

static int timer_cancel_wake_at(void *id);

typedef struct thread_queue {
    thread_t *head;
    thread_t *tail;
} ThreadQueue;

#define IDLE_STACK 4096*3                       // The idle thread stack size.
static thread_t idle_thread[CONFIG_PROCESSORS]; // The idle threads.
static char idle_stack[CONFIG_PROCESSORS][IDLE_STACK]
  __attribute__((aligned(8)));

static void schedule_nolock(thread_t *list);

/* The tid_mutex protects the thread id pool.
 */
static pthread_mutex_t tid_mutex = PTHREAD_MUTEX_INITIALIZER;
static int tids[CONFIG_THREADS];
static int *tid_in;
static int *tid_out;

/** The thread id to thread mapping.
 */
static thread_t *threads[CONFIG_THREADS];

/** Initialize the tid pool.
 */
static void tid_initialize(void)
{
  int i = 0;
  for (int tid = 0; tid < CONFIG_THREADS; ++tid) {
    if (tid == 1) {
      // Reserve tid 1 for a future spawn of init.
      continue;
    }

    tids[i++] = tid;
  }

  tid_in = &tids[i];
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
  if (tid_out == &tids[CONFIG_THREADS]) {
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
  if (tid < 0 || tid >= CONFIG_THREADS) {
    // This should never happen.
    return;
  }

  pthread_mutex_lock(&tid_mutex);
  if (tid_in == &tids[CONFIG_THREADS]) {
    tid_in = tids;
  }

  if (tid_in == tid_out) {
    // The tid pool is full.
    // This should never happen.
    pthread_mutex_unlock(&tid_mutex);
    return;
  }

  *tid_in++ = tid;
  // Clear the thread map entry.
  threads[tid] = NULL;
  pthread_mutex_unlock(&tid_mutex);
}

/** The idle thread.
 */
static void idle(void)
{
  for ( ;; ) {
    // Wait for the next interrupt.
    suspend();
  }
}

/** Create an idle thread for each processor.
 * These threads can never exit.
 */
static void create_system_threads(void)
{
  for (int i = 0; i < CONFIG_PROCESSORS; ++i) {
    char name[20];
    context_t *ctx = (context_t *)&idle_stack[i][IDLE_STACK];
    idle_thread[i].context = ctx;
    --ctx;
    new_context(&idle_thread[i].context, idle, INITIAL_PSR, 0,
                (char *)ctx, 0);
    idle_thread[i].priority = CONFIG_PRIORITIES;        // The lowest priority.
    idle_thread[i].state = IDLE;
    alloc_tid(&idle_thread[i]);
    int s = snprintf(name, 20, "idle%d", i) + 1;
    idle_thread[i].name = kmem_alloc(s);
    strcpy(idle_thread[i].name, name);
  }
}

/* The ready_lock protects the following variables.
 */
static lock_t ready_lock;

static int priority;                    // The current highest priority.

#if CONFIG_PRIORITIES > 1 && CONFIG_PROCESSORS > 1
// Multiple priorities and processors.
static int processor() { return 0; }            // RICH: For now.
static void *slice_tmo[CONFIG_PROCESSORS];      // Time slice timeout ID.
static ThreadQueue ready[CONFIG_PRIORITIES];
static long irq_state[CONFIG_PROCESSORS];       // Set if running in irq state.

#define slice_tmo slice_tmo[processor()]
#define ready_head(pri) ready[pri].head
#define ready_tail(pri) ready[pri].tail
#define irq_state irq_state[processor()]

#elif CONFIG_PROCESSORS > 1
// Multiple processors, one priority.
static int processor() { return 0; }            // RICH: For now.
static void *slice_tmo[CONFIG_PROCESSORS];      // Time slice timeout ID.
static ThreadQueue ready;
static long irq_state[CONFIG_PROCESSORS];       // Set if running in irq state.

#define slice_tmo slice_tmo[processor()]
#define ready_head(pri) ready.head
#define ready_tail(pri) ready.tail
#define irq_state irq_state[processor()]

#elif CONFIG_PRIORITIES > 1
// One processor, multiple priorities.
static void *slice_tmo;                 // Time slice timeout ID.
static ThreadQueue ready[CONFIG_PRIORITIES];
static long irq_state;                  // Set if running in irq state.

#define processor() 0
#define slice_tmo slice_tmo
#define ready_head(pri) ready[pri].head
#define ready_tail(pri) ready[pri].tail

#else
// One processor, one priority.
static void *slice_tmo;                 // Time slice timeout ID.
static ThreadQueue ready;               // The ready to run list.
static long irq_state;                  // Set running in irq state.

#define processor() 0
#define slice_tmo slice_tmo
#define ready_head(pri) ready.head
#define ready_tail(pri) ready.tail

#endif

// Threads currently assigned to processors.
static thread_t *current[CONFIG_PROCESSORS];
#define current current[processor()]

#define idle_thread idle_thread[processor()]

static long long slice_time = 5000000;  // The time slice period (ns).

/**** End of ready lock protected variables. ****/

#if CONFIG_ENABLEFDS
static struct fs main_thread_fs = {
  .lock = PTHREAD_MUTEX_INITIALIZER,
  .refcnt = 1,
  .cwd = NULL,
  .root = NULL,
  .umask = 0
};
#endif

#define MAIN_STACK 4096*4       // The main "thread" stack size.
static char main_stack[MAIN_STACK] __attribute__((aligned(8)));

// The main thread's initial values.
static thread_t main_thread = {
  .context = (context_t *)&main_stack[MAIN_STACK],
  .name = "kernel",
  .priority = DEFAULT_PRIORITY,
  .state = RUNNING,

#if CONFIG_ENABLEFDS
  .fs = &main_thread_fs,
#endif

#if CONFIG_HAVE_CAPABILITY
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
  if (pid < 0 || pid >= CONFIG_THREADS || threads[pid] == NULL) {
    return 0;
  }

  return 1;
}

#if CONFIG_VM
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

#if RICH
/** Lock the ready queue.
 */
static inline void lock_ready(void)
{
    lock_acquire(&ready_lock);
}
#endif

/** Unlock the ready queue.
 * This function is called from crt1.S.
 */
void unlock_ready(void)
{
    lock_release(&ready_lock);
}

/** Enter the irq state.
 * This function is called from crt1.S.
 */
void *enter_irq(void)
{
  // RICH: lock_acquire(&ready_lock);
  long state = irq_state++;
  if (state) return NULL;               // Already in irq state.
  return current;                       // To save context.
}

/** Leave the irq state.
 * This function is called from crt1.S.
 */
void *leave_irq(void)
{
  long state = --irq_state;
  if (state) return NULL;               // Still in irq state.
  lock_acquire(&ready_lock);
  return current;                       // Next context.
}

/** Get the current thread pointer.
 * This function is called from crt1.S.
 */
thread_t *thread_self()
{
  if (irq_state) return NULL;           // Nothing current if in irq state.
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
  lock_acquire(&ready_lock);
  if ((thread_t *)arg1 == current) {
    schedule_nolock((thread_t *)arg1);
    return;
  }
  lock_release(&ready_lock);
}

/** Make the head of the ready list the running thread.
 * The ready lock must be acquired before this call.
 */
static void get_running(void)
{
  int timeslice = 1;  // Assume a time slice is needed.

  // Find the highest priority thread to run.
  current = priority < CONFIG_PRIORITIES ? ready_head(priority) : NULL;

#if CONFIG_PRIORITIES > 1
  // Check for lower priority things to run.
  while (current == NULL && priority < CONFIG_PRIORITIES) {
    ++priority;
    current = ready_head(priority);
  }
#endif

  if (current == NULL) {
    timeslice = 0;          // No need to timeslice for the idle thread.
    current = &idle_thread;
    priority = CONFIG_PRIORITIES;
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
 * The ready lock has been acquired.
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
  switch_context(&current->context, &me->context);
}

/* Schedule a list of threads.
 */
static void schedule(thread_t *list)
{
  lock_acquire(&ready_lock);
  schedule_nolock(list);
}

/** Delete a thread.
 * The thread has been removed from all lists.
 */
static void thread_delete(thread_t *tp)
{
  release_tid(tp->tid);

#if CONFIG_ENABLEFDS
  fdset_release(tp->fdset);
  pthread_mutex_lock(&tp->fs->lock);
  ASSERT(tp->fs->refcnt > 0);
  if (--tp->fs->refcnt == 0) {
    if (tp->fs->cwd)
      vrele(tp->fs->cwd);
    if (tp->fs->root)
      vrele(tp->fs->root);
    pthread_mutex_unlock(&tp->fs->lock);
    pthread_mutex_destroy(&tp->fs->lock);
    kmem_free(tp->fs);
  } else {
    pthread_mutex_unlock(&tp->fs->lock);
  }
#endif
}

/** Change the current thread's state to
 * something besides READY or RUNNING.
 */
static int change_state(int arg, state new_state)
{
  if (current->state == IDLE) {
    // Idle threads can't change state.
    return -EINVAL;
  }

  lock_acquire(&ready_lock);
  if (new_state == EXITING) {
    // Remove from the ready list.
    thread_t *me = current;
    current = current->next;
    get_running();
    thread_delete(me);
    return enter_context(me, thread_free, current->context);
  }

  thread_t *me = current;
  me->state = new_state;
  get_running();
  return switch_context_arg(arg, &current->context, &me->context);
}

/* Give up the remaining time slice.
 */
static int sys_sched_yield(void)
{
  schedule(current);
  return 0;
}

/* Set pointer to thread ID.
 * @param tidptr Where to put the thread ID.
 * @return The tid of the calling process.
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
static struct timeout *timeouts;        // Active timeouts.
static struct timeout *free_timeouts;   // Freed timeouts.

/** Make an entry in the sleeping list and sleep
 * or schedule a callback.
 * RICH: Mark realtime timers.
 */
void *timer_wake_at(long long when,
                    TimerCallback callback, void *arg1, void *arg2, int retval)
{
  struct timeout *tmo = NULL;

  lock_acquire(&timeout_lock);
  if (free_timeouts) {
    tmo = free_timeouts;
    free_timeouts = tmo->next;
  }
  lock_release(&timeout_lock);

  if (tmo == NULL) {
    tmo = kmem_alloc(sizeof(struct timeout));
    if (tmo == NULL) {
      return NULL;
    }
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

  lock_acquire(&timeout_lock);
  // Search the list.
  int head = 0;
  if (timeouts == NULL) {
    timeouts = tmo;
    head = 1;
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
      head = 1;
    }
  }

  lock_release(&timeout_lock);
  if (head) {
    // Inserted at the head of the list, reset timer.
    timer_start(when);
  }

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

/** Timeouts must be locked for this call.
 */
static void timeout_free(struct timeout *tp)
{
  tp->next = free_timeouts;
  free_timeouts = tp;
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
  lock_acquire(&timeout_lock);
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
    lock_release(&timeout_lock);

    if (p->waiter) {
      // Wake up the sleeping thread.
      p->waiter->next = NULL;
      schedule(p->waiter);
    }

    lock_acquire(&timeout_lock);
    timeout_free(p);
    lock_release(&timeout_lock);
  } else {
    lock_release(&timeout_lock);
    s = -1;                 // Not found.
  }

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
                                   void *uaddr2, int retval)
{
  int s = 0;
  lock_acquire(&timeout_lock);
  struct timeout *p, *q;
  for (p = timeouts, q = NULL; p; q = p, p = p->next) {
    if (p->arg1 == arg1 && p->arg2 == arg2) {
      // Have a match.
      if (s < count) {
	// Remove this entry.
	if (q) {
	  q->next = p->next;
	} else {
	  timeouts = p->next;
	}
	lock_release(&timeout_lock);

	if (p->waiter) {
	  // Wake up the sleeping thread.
	  p->waiter->next = NULL;
	  context_set_return(p->waiter->context, retval);
	  schedule(p->waiter);
	  ++s;
	  lock_acquire(&timeout_lock);
	  timeout_free(p);
	  lock_release(&timeout_lock);
	}
      } else if (uaddr2) {
	// Wait on a new address.
	p->arg2 = uaddr2;
	++s;
      } else {
	if (++s >= count) {
	  lock_acquire(&timeout_lock);
	  timeout_free(p);
	  lock_release(&timeout_lock);
	  break;
	}
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
  lock_acquire(&timeout_lock);
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
      TimerCallback callback = tmo->callback;
      void *arg1 = tmo->arg1;
      void *arg2 = tmo->arg2;
      lock_release(&timeout_lock);
      callback(arg1, arg2);
      lock_acquire(&timeout_lock);
    }

    timeout_free(tmo);
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

#if !CONFIG_VM
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
                      void *tls, int *ctid,
#elif defined(__i386__) || defined(__x86_64__)
                      int *ctid, void *tls,
#else
  #error clone arguments not defined
#endif
                      long arg5, long data, void (*entry)(void))
{
  // Create a new thread, copying context.
  int s;
  thread_t *tp;
  tp = THRD_ALLOC(tp);
  if (tp == NULL) {
    return -ENOMEM;
  }

  // Inherit the parent's attributes.
  *tp = *current;
  tp->clear_child_tid = NULL;

  // Mark the parent.
  tp->ppid = tp->tid;

  int tid = alloc_tid(tp);
  if (tid < 0) {
    thread_free(tp);
    return -EAGAIN;
  }

#if CONFIG_VM
  if (flags & CLONE_VM) {
    // Share the address space of the parent.
    vm_reference(tp->map);
  } else {
    // This thread will have a new address space.
    tp->map = vm_dup(tp->map);
    if (tp->map == NULL) {
      // The dup failed.
      release_tid(tid);
      thread_free(tp);
      return -ENOMEM;
    }
  }
#endif

#if CONFIG_ENABLEFDS
  // Clone or copy the file descriptor set.
  s = fdset_clone(&tp->fdset, flags & CLONE_FILES);
  if (s < 0) {
#if CONFIG_VM
    vm_terminate(tp->map);
#endif
    release_tid(tid);
    thread_free(tp);
    return s;
  }

  if (flags & CLONE_FS) {
    // Share file system information.
    pthread_mutex_lock(&tp->fs->lock);
    ++tp->fs->refcnt;
    pthread_mutex_unlock(&tp->fs->lock);
  } else {
    // Start a new file system context.
    fs_t pfs = tp->fs;                  // Get the parent fs.
    pthread_mutex_lock(&pfs->lock);
    tp->fs = kmem_alloc(sizeof(*tp->fs));
    if (tp->fs == NULL) {
      pthread_mutex_unlock(&pfs->lock);
#if CONFIG_VM
      vm_terminate(tp->map);
#endif
      release_tid(tid);
      thread_free(tp);
      return -ENOMEM;
    }

    *tp->fs = *pfs;
    pthread_mutex_init(&tp->fs->lock, NULL);
    tp->fs->refcnt = 1;
    if (tp->fs->cwd)
      vref(tp->fs->cwd);
    if (tp->fs->root)
      vref(tp->fs->root);
    pthread_mutex_unlock(&pfs->lock);
  }
#endif

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

#if RICH
  context_t *cp = (context_t *)stack;
#else
  context_t *cp = (context_t *)((char *)tp + CONFIG_THREAD_SIZE);
#endif
  tp->context = cp;
  // Copy registers.
  *--cp = *current->context;

  new_context(&tp->context, entry, INITIAL_PSR, 0, stack, tls);

  // Schedule the thread.
  schedule(tp);
  // Do not access *tp here: The thread may have already exited.
  return tid;
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

  case FUTEX_REQUEUE:
    s = timer_cancel_wake_count(val, (void *)FUTEX_MAGIC, uaddr, uaddr2, 0);
    break;
  case FUTEX_WAKE:
    s = timer_cancel_wake_count(val, (void *)FUTEX_MAGIC, uaddr, NULL, 0);
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
  if (tid < 0 || tid >= CONFIG_THREADS) {
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
  if (tid < 0 || tid >= CONFIG_THREADS) {
    return -EINVAL;
  }

  thread_t *tp = threads[tid];
  if (tp == NULL) {
    // Invalid thread id.
    return -ESRCH;
  }

  lock_acquire(&ready_lock);
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

char *get_brk(pid_t pid)
{
  if (pid == 0) {
    pid = current->pid;
  }

  if (pid < 0 || pid >= CONFIG_THREADS) {
    return NULL;
  }

  thread_t *tp = threads[pid];
  if (tp == NULL) {
    // Invalid process id.
    return NULL;
  }

  return tp->brk;
}

void set_brk(pid_t pid, char *brk)
{
  if (pid == 0) {
    pid = current->pid;
  }

  if (pid < 0 || pid >= CONFIG_THREADS) {
    return;
  }

  thread_t *tp = threads[pid];
  if (tp == NULL) {
    // Invalid process id.
    return;
  }

  tp->brk = brk;
}

static void sys_exit(int status)
{
  if (current->clear_child_tid) {
    *current->clear_child_tid = 0;
    timer_cancel_wake_count(~0, (void *)FUTEX_MAGIC, current->clear_child_tid,
                            NULL, 0);
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

  if (pid < 0 || pid >= CONFIG_THREADS || threads[pid] == NULL) {
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
  if (pgid < 0 || pgid >= CONFIG_THREADS) {
    return -EINVAL;
  }

  // Check for a valid pid.
  if (pid < 0 || pid >= CONFIG_THREADS) {
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
#if CONFIG_HAVE_CAPABILITY
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

#if CONFIG_HAVE_CAPABILITY
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
  mode_t old = current->fs->umask;
  current->fs->umask = new;
  return old;
}

#if CONFIG_SIGNALS

static int sys_rt_sigprocmask(int how, const sigset_t *set, sigset_t *oldset)
{
  int s = 0;
  if (oldset) {
    s = copyout(&current->sigmask, oldset, sizeof(sigset_t));
    if (s != 0) {
      return s;
    }
  }

  if (set == NULL) {
    return 0;
  }

  sigset_t tmp;
  switch(how) {
    case SIG_BLOCK:
      s = copyin(set, &tmp, sizeof(sigset_t));
      if (s == 0) {
        for (int i = 0; i < sizeof(sigset_t) / sizeof(unsigned long); ++i) {
          current->sigmask.__bits[i] |= tmp.__bits[i];
        }
      }
      break;
    case SIG_UNBLOCK:
      s = copyin(set, &tmp, sizeof(sigset_t));
      if (s == 0) {
        for (int i = 0; i < sizeof(sigset_t) / sizeof(unsigned long); ++i) {
          current->sigmask.__bits[i] &= ~tmp.__bits[i];
        }
      }
    case SIG_SETMASK:
      s = copyin(set, &current->sigmask, sizeof(sigset_t *));
      break;
    default:
      s = -EINVAL;
      break;
  }

  return 0;
}

#endif

#if CONFIG_ENABLEFDS
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

/** Replace the old cwd with a new one.
 */
void replacecwd(vnode_t vp)
{
  vnode_t oldvp = current->fs->cwd;
  vref(vp);
  current->fs->cwd = vp;
  if (oldvp) {
    vrele(oldvp);
  }
}

/** Replace the old root with a new one.
 */
void replaceroot(vnode_t vp)
{
  vnode_t oldvp = current->fs->root;
  vref(vp);
  current->fs->root = vp;
  if (oldvp) {
    vrele(oldvp);
  }
}

/** Get a file path.
 * This function returns the full path name for the file name.
 * If full is set, the real file name is returned, otherwise
 * the name is relative to the chroot() directory.
 */
int getpath(const char *name, char *path, int full)
{
  // Find the current directory name.
  const char *root = current->fs->root ? current->fs->root->v_path : "/";
  int rootlen = strlen(root);
  const char *cwd = current->fs->cwd ? current->fs->cwd->v_path : "/";
  const char *src = name;
  char *tgt = path;
  int len = 0;
  if (src[0] == '/') {
    // The path starts at the root.
    ++src;
    if (full) {
      strlcpy(tgt, root, rootlen + 1);
      tgt += rootlen;
      len = rootlen;
    } else {
      // A chroot() relative path name.
      *tgt++ = '/';
      ++len;
    }

    if (len > 1 && *src != '\0' && *src != '.') {
      if (++len >= PATH_MAX)
        return -ENAMETOOLONG;
      *tgt++ = '/';
      ++len;
    }
  } else {
    // The current working directory starts the path.
    cwd += (full || rootlen == 1 ? 0 : rootlen);
    if (*cwd) {
      strlcpy(tgt, cwd, PATH_MAX);
      len = strlen(cwd);
    } else {
      // At the root of a chroot().
      *tgt = '/';
      len = 1;
    }
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
      if (*src == '.') {
        if (++len >= PATH_MAX)
          return -ENAMETOOLONG;
        *tgt++ = '/';
      }

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

#endif // CONFIG_ENABLEFDS

#if CONFIG_THREAD_COMMANDS

static int psCommand(int argc, char **argv)
{
  if (argc <= 0) {
    printf("show process information.\n");
    return COMMAND_OK;
  }

  int context = 0;
  while (argc > 1) {
    const char *p = argv[1];
    if (*p++ != '-') {
      break;
    }

    while (*p) {
      switch (*p) {
      case 'c':
        context = 1;
        break;
      default:
        fprintf(stderr, "unknown option character '%c'\n", *p);
        return COMMAND_ERROR;
      }
      ++p;
    }

    --argc;
    ++argv;
  }

  if (argc != 1) {
    fprintf(stderr, "invalid argument \"%s\"\n", argv[1]);
    return COMMAND_ERROR;
  }

  struct meminfo meminfo;
  page_info(&meminfo);
  printf("Total pages: %lu (%lu bytes), Free pages: %lu (%lu bytes)\n",
         meminfo.total / PAGE_SIZE, meminfo.total,
         meminfo.free / PAGE_SIZE, meminfo.free);
  int heading = 1;
  for (int i = 0;  i < CONFIG_THREADS; ++i) {
    thread_t *t =  threads[i];
    if (t == NULL) {
      continue;
    }
    if (heading) {
      printf("%6.6s %6.6s %10.10s %-10.10s %5.5s %-10.10s \n",
             "PID", "TID", "TADR", "STATE", "PRI", "NAME");
      heading = 0;
    }
    printf("%6d ", t->pid);
    printf("%6d ", t->tid);
    printf("%8p ", t);
    printf("%-10.10s ", state_names[t->state]);
    printf("%5d ", t->priority);
    printf(t->pid == t->tid ? "%s" : "[%s]", t->name ? t->name : "");
    printf("\n");
    if (context) {
      if (t->state == RUNNING) {
        printf("Thread is running\n");
      } else {
        trap_dump(printf, "Context", t->context);
      }
      heading = 1;
    }
  }

  return COMMAND_OK;
}

static int ssCommand(int argc, char **argv)
{
  if (argc <= 0) {
    printf("show sleeping thread information.\n");
    return COMMAND_OK;
  }

  lock_acquire(&timeout_lock);
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

#endif  // CONFIG_THREAD_COMMANDS

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
#if CONFIG_VM
extern char __end[];            // The end of the .bss area.

struct bootinfo bootinfo;
#endif

/** An optiona system initialization function.
 * Called from __elk_start() before C library initialization.
 */
static void init(void)
{
#if CONFIG_VM

// RICH: Move this.
  bootinfo.nr_rams = 2;
  bootinfo.ram[0].base = 0x48000000;
  bootinfo.ram[0].size = 0x02000000;
  bootinfo.ram[0].type = MT_USABLE;
  bootinfo.ram[1].base = 0x48000000;
  bootinfo.ram[1].size = round_page((uintptr_t)__end) - SYSPAGE;
  bootinfo.ram[1].type = MT_RESERVED;

// RICH: The size of the kernel kmem heap.
#define KMEM_SIZE (8 * PAGE_SIZE)

  diag_init();
  page_init();
  kmem_init(KMEM_SIZE);

  // Initialize the MMU page map.
  vm_mmu_init();

  // Reserve system pages.
  // page_reserve(kvtop(SYSPAGE), SYSPAGESZ);

  // Initialize the cache.
  cache_init();

#endif


#if RICH
  machine_startup();            // Target dependent initialzation.
#endif

#if CONFIG_VM
  // Set up the kernel memory map.
  current->map = vm_init(KMEM_SIZE);
#endif
}

/** An optional system initialization function.
 * Called from __elk_start() after C library initialization.
 */
static void c_init(void)
{
#if CONFIG_ENABLEFDS
  int s;
  // Set up the RAM file system.
  s = mount("", "/", "ramfs", 0, NULL);
  if (s) {
    DPRINTF(MSG, ("ramfs mount failed: %s\n", strerror(errno)));
  }

  // Create and mount the defice directory.
  s = mkdir("/dev", S_IRWXU);
  if (s) {
    DPRINTF(MSG, ("/dev mkdir failed: %s\n", strerror(errno)));
  }
  s = mount("", "/dev", "devfs", 0, NULL);
  if (s) {
    DPRINTF(MSG, ("devfs mount failed: %s\n", strerror(errno)));
  }

  // Create the kernel stdin, stdout, and stderr.
  int fd = open("/dev/tty", O_RDWR);
  if (fd) {
    DPRINTF(MSG, ("open(/dev/tty) failed: %s\n", strerror(errno)));
  }
  s = dup2(fd, 0);
  if (s < 0) {
    DPRINTF(MSG, ("dup2(%d, 0) failed: %s\n", fd, strerror(errno)));
  }
  s = dup2(fd, 1);
  if (s < 0) {
    DPRINTF(MSG, ("dup2(%d, 1) failed: %s\n", fd, strerror(errno)));
  }
  s = dup2(fd, 2);
  if (s < 0) {
    DPRINTF(MSG, ("dup2(%d, 2) failed: %s\n", fd, strerror(errno)));
  }
  if (fd != 0)
      close(fd);
#endif
}

/* Initialize the thread handling code.
 */
ELK_PRECONSTRUCTOR()
{
  /** We set up a thread_self pointer early to tell
   * the C library that we support threading.
   */
  main_thread.brk = (char *)round_page((uintptr_t)__end + KMEM_SIZE);
  current = &main_thread;

#if CONFIG_THREAD_COMMANDS
  command_insert(NULL, sectionCommand);
  command_insert("ps", psCommand);
  command_insert("ss", ssCommand);
#endif // CONFIG_THREAD_COMMANDS
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
#if CONFIG_SIGNALS
  SYSCALL(rt_sigprocmask);
#endif
  SYSCALL(tkill);
  SYSCALL(umask);

  // Set up the system initialization functions.
  system_init = init;
  system_c_init = c_init;
  // Set up the tid pool.
  tid_initialize();

  // The main thread is what's running right now.
  alloc_tid(current);

  current->pid = current->tid;          // The main thread starts a group.
  priority = current->priority;

}

C_CONSTRUCTOR()
{
#if CONFIG_ENABLEFDS
  int s = fdset_new(&current->fdset);
  ASSERT(s == 0);
#endif

  // Create the system thread(s).
  create_system_threads();
}
