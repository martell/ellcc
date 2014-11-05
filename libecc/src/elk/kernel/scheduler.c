/** Schedule threads for execution.
 */
#include <bits/syscall.h>       // For syscall numbers.
#define _GNU_SOURCE
#include <stdio.h>
#include <sched.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <inttypes.h>
#include <timer.h>
#define DEFINE_STRINGS
#include <scheduler.h>
#include <command.h>

// Make the scheduler a loadable feature.
FEATURE(scheduler, scheduler)

#define THREADS 1024                    // The number of threads supported.

#define PRIORITIES 3                    // The number of priorities to support:
                                        // (0..PRIORITIES - 1). 0 is highest.
#define DEFAULT_PRIORITY ((PRIORITIES)/2)
#define PROCESSORS 1                    // The number of processors to support.

#define MIN_STACK   4096                // The minimum stack size for a thread.

typedef struct thread_queue {
    __elk_thread *head;
    __elk_thread *tail;
} ThreadQueue;

#define IDLE_STACK 4096                 // The idle thread stack size.
static __elk_thread idle_thread[PROCESSORS];  // The idle threads.
static char *idle_stack[PROCESSORS][IDLE_STACK];

static void schedule_nolock(__elk_thread *list);

/* The tid_lock protects the thread id pool.
 */
static __elk_lock tid_lock;
static int tids[THREADS];
static int *tid_in;
static int *tid_out;

/** The thread id to thread mapping.
 */
static __elk_thread *threads[THREADS];

/** Initialize the tid pool.
 */
static void tid_initialize(void)
{
  for (int tid = 0; tid < THREADS; ++tid) {
    tids[tid] = tid;
  }

  tid_in = &tids[THREADS];
  tid_out = &tids[0];
}

/** Allocate a thread id to a new thread.
 */
static int alloc_tid(__elk_thread *tp)
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

#if RICH
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
#endif

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
 */
static void create_idle_threads(void)
{
  for (int i = 0; i < PROCESSORS; ++i) {
    idle_thread[i].saved_ctx = (__elk_context *)&idle_stack[i][IDLE_STACK];
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
static __elk_lock ready_lock;

static int priority;                    // The current highest priority.

#if PRIORITIES > 1 && PROCESSORS > 1
// Multiple priorities and processors.
static int processor() { return 0; }    // RICH: For now.
static __elk_thread *current[PROCESSORS];
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
static __elk_thread *current[PROCESSORS];
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
static __elk_thread *current;           // The current running thread.
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
static __elk_thread *current;           // The current running thread.
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

static __elk_thread main_thread;        // The main thread.

/** Get the current thread id.
 */
int gettid(void)
{
  return current->tid;
}

/** Enter the IRQ state.
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
 */
void *__elk_leave_irq(void)
{
  __elk_lock_aquire(&ready_lock);
  long state = --irq_state;
  if (state) return 0;                // Still in IRQ state.
  return current;                     // Next context.
}

/** Get the current thread pointer.
 */
__elk_thread *__elk_thread_self()
{
  return current;
}

/* Insert a thread in the ready queue.
 */
static inline void insert_thread(__elk_thread *thread)
{
  // A simple FIFO insertion.
  if (ready_tail(thread->priority)) {
    ready_tail(thread->priority)->next = thread;
  } else {
    ready_head(thread->priority) = thread;
  }
  ready_tail(thread->priority) = thread;
  thread->next = NULL;
  thread->state = READY;
  if (priority > thread->priority) {
    // A higher priority is ready.
    priority = thread->priority;
  }
}

/** The callback for time slice expiration.
 * @param arg The thread being timed.
 */
static void slice_callback(void *arg1, void *arg2)
{
  __elk_lock_aquire(&ready_lock);
  if ((__elk_thread *)arg1 == current) {
    schedule_nolock((__elk_thread *)arg1);
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
    long long when = timer_get_monotonic(); // Get the current time.
    when += slice_time;                     // Add the slice time.
    slice_tmo = timer_wake_at(when, slice_callback, (void *)current, 0);
  }

  current->state = RUNNING;
  current->next = NULL;
}

/* Schedule a list of threads.
 * The ready lock has been aquired.
 */
static void schedule_nolock(__elk_thread *list)
{
  __elk_thread *next;

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
  __elk_thread *me = current;
  get_running();
  __elk_switch(&current->saved_ctx, &me->saved_ctx);
}

/* Schedule a list of threads.
 */
void __elk_schedule(__elk_thread *list)
{
  __elk_lock_aquire(&ready_lock);
  schedule_nolock(list);
}

/** Change the current thread's state to
 * something besides READY or RUNNING.
 * The ready list must be locked on entry.
 */
static int nolock_change_state(int arg, __elk_state new_state)
{
  __elk_thread *me = current;
  me->state = new_state;
  get_running();
  return __elk_switch_arg(arg, &current->saved_ctx, &me->saved_ctx);
}

/** Change the current thread's state to
 * something besides READY or RUNNING.
 */
int __elk_change_state(int arg, __elk_state new_state)
{
  __elk_lock_aquire(&ready_lock);
  return nolock_change_state(arg, new_state);
}

/* Give up the remaining time slice.
 */
static int sys_sched_yield(void)
{
  __elk_schedule(current);
  return 0;
}

/* Create a new thread.
 */
static int thread_create_int(const char *name, __elk_thread **id,
                             void (*entry)(void), int priority,
                             int cloning, void *stack, size_t size,
                             intptr_t arg)
{
  char *p;
  if (stack == 0) {
    if (size < MIN_STACK) {
      size = MIN_STACK;
    }

    p = malloc(size);

    if (p == NULL) {
      return -ENOMEM;
    }

    p += size;
  } else {
    p = stack + size;
  }

  __elk_thread *thread = malloc(sizeof(__elk_thread));    // RICH: bin.
  if (!thread) {
    if (!stack) free(p);
    return -ENOMEM;
  }

  int s = alloc_tid(thread);
  if (s < 0) {
    if (!stack) free(p);
    free(thread);
  }

  thread->next = NULL;
  thread->tls = NULL;
  if (priority == 0) {
    priority = DEFAULT_PRIORITY;
  } else if (priority >= PRIORITIES) {
    priority = PRIORITIES - 1;
  }
  thread->priority = priority;

  thread->set_child_tid = NULL;
  thread->clear_child_tid = NULL;
  thread->queue = (MsgQueue)MSG_QUEUE_INITIALIZER;
  if (name) {
    thread->name = strdup(name);
  } else {
    thread->name = NULL;
  }

  __elk_context *cp = (__elk_context *)p;
  thread->saved_ctx = cp;
  if (cloning) {
    // Copy registers for clone();
    *(cp - 1) = *current->saved_ctx;
  }

  __elk_new_context(&thread->saved_ctx, entry, INITIAL_PSR, arg);
  *id = thread;
  return 0;
}

/** Send a message to a message queue.
 */
int __elk_send_message_q(MsgQueue *queue, Message msg)
{
  if (queue == NULL) {
    queue = &__elk_thread_self()->queue;
  }
  Envelope *envelope = (Envelope *)malloc(sizeof(Envelope));
  if (!envelope) {
    return ENOMEM;
  }
  envelope->message = msg;

  __elk_thread *wakeup = NULL;
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
    __elk_schedule(wakeup);
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
  __elk_thread *thread = threads[tid];

  if (thread == NULL) {
    return ESRCH;
  }

  return __elk_send_message_q(&thread->queue, msg);
}

Message get_message(MsgQueue *queue)
{
  if (queue == NULL) {
    queue = &__elk_thread_self()->queue;
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
      __elk_thread *me = current;
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
    queue = &__elk_thread_self()->queue;
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
  static int number = 1;
  char name[20];
  snprintf(name, 20, "clone%d", number++);
  __elk_thread *new;
  int s = thread_create_int(name, &new, entry, 0, 1, stack, 0, 0);
  if (s < 0) {
    return s;
  }

  // Record the TLS.
  new->tls = regs;

  if (flags & CLONE_CHILD_CLEARTID) {
    VALIDATE_ADDRESS(ctid, sizeof(*ctid), VALID_RD);
    new->clear_child_tid = ctid;
  }

  if (flags & CLONE_CHILD_SETTID) {
    VALIDATE_ADDRESS(ctid, sizeof(*ctid), VALID_WR);
    new->set_child_tid = ctid;
    // RICH: This write should be in the child's address space.
    *ctid = new->tid;
  }

  if (flags & CLONE_PARENT_SETTID) {
    VALIDATE_ADDRESS(ptid, sizeof(*ptid), VALID_WR);
    // RICH: This write should also be in the child's address space.
    *ptid = new->tid;
  }

  __elk_schedule(new);
  return new->tid;
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

  __elk_thread *thread = threads[tid];
  if (thread == NULL) {
    // Invalid thread id.
    return -ESRCH;
  }

  __elk_lock_aquire(&ready_lock);
  if (thread == current) {
    // The currently running thread is being signaled.
  } else if (thread->state == READY) {
    // Remove this thread from its ready list.
    __elk_thread *p, *l;
    for (p = ready_head(thread->priority), l = NULL;
         p != thread; l = p, p = p->next) {
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
      ready_head(thread->priority) = p->next;
    }
  } else {
    printf("RICH: need to handle non-current, non-ready threads\n");
  }

  // Get the next runnable thread.
  // RICH: get_running();
  __elk_lock_release(&ready_lock);
  return 0;
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
    __elk_thread *t =  threads[i];
    if (t == NULL) {
      continue;
    }
    printf("%6d ", t->pid);
    printf("%6d ", t->tid);
    printf("%8p ", t);
    printf("%-10.10s ", state_names[t->state]);
    printf("%5d ", t->priority);
    printf("%-10.10s ", t->name ? t->name : "");
    printf("\n");
  }

  return COMMAND_OK;
}

/* Initialize the scheduler.
 */
CONSTRUCTOR()
{
  // Set up the tid pool.
  tid_initialize();

  // Set up the main and idle threads.

  // The main thread is what's running right now.
  main_thread.priority = DEFAULT_PRIORITY;
  main_thread.state = RUNNING;
  main_thread.next = NULL;
  main_thread.name = "kernel";
  alloc_tid(&main_thread);
  main_thread.pid = main_thread.tid;    // The main thread starts a group.
  main_thread.set_child_tid = NULL;
  main_thread.clear_child_tid = NULL;
  priority = main_thread.priority;
  current = &main_thread;

  create_idle_threads();

  // Add the "ts" command.
  command_insert("ts", tsCommand);

  // Set up a set_tid_address system call.
  __elk_set_syscall(SYS_set_tid_address, sys_set_tid_address);

  // Set up the sched_yield system call.
  __elk_set_syscall(SYS_sched_yield, sys_sched_yield);
  // Set up the clone system call.
  __elk_set_syscall(SYS_clone, sys_clone);
  // Set up the futex system call.
  __elk_set_syscall(SYS_futex, sys_futex);
  // Set up the tkill system call.
  __elk_set_syscall(SYS_tkill, sys_tkill);
}
