/** Thread definitions.
 */
#ifndef _thread_h_
#define _thread_h_

#include <limits.h>

#include "config.h"
#include "kernel.h"
#include "vnode.h"
#include "file.h"
#include "vm.h"
#include "hal.h"

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
#define replaceroot __elk_replaceroot
#define getpath __elk_getpath
#define gettid __elk_gettid
#define getpid __elk_getpid
#define getmap __elk_getmap
#define getcurmap __elk_getcurmap
#define pid_valid __elk_pid_valid
#define signal_post __elk_signal_post
#define sched_dpc __elk_sched_dpc
#define get_brk __elk_get_brk
#define set_brk __elk_set_brk
#endif

#if !CONFIG_HAVE_CAPABILITY
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

// Allow network administration.
#define CAP_NET_ADMIN 12

// Allow raw I/O.
#define CAP_SYS_RAWIO 17

// Allow chroot().
#define CAP_SYS_CHROOT 18

// Allow ptrace and memory access of any process.
#define CAP_SYS_PTRACE 19

// Allow mount, umount.
#define CAP_SYS_ADMIN 21

#define CAPABILITY_TO_BIT(capability) (1 << (capability))

typedef int capability_t;

int capable(capability_t cap);

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
long long timer_expired(long long);

/** Make an entry in the sleeping list and sleep
 * or schedule a callback.
 */
typedef void (*TimerCallback)(void *, void *);
void *timer_wake_at(long long, TimerCallback, void *, void *, int);

struct file;

/** Get a file pointer corresponding to a file descriptor.
 */
int getfile(int, struct file **);

/** Get a duplicate file descriptor.
 */
int getdup(int, struct file **, int);

/** Get a file descriptor.
 */
int allocfd(file_t);

/** Set a file pointer corresponding to a file descriptor.
 */
int setfile(int, struct file *);

/** Replace the old cwd with a new one.
 */
void replacecwd(vnode_t);

/** Replace the old root with a new one.
 */
void replaceroot(vnode_t);

/** Get a file path.
 * This function returns the full path name for the file name.
 */
int getpath(const char *, char *, int);

/** Get the current thread id.
 */
int gettid(void);

/** Get the current process id.
 */
int getpid(void);

/** Get the memory map of a process.
 */
vm_map_t getmap(pid_t);

/** Get the memory map of the current process.
 */
vm_map_t getcurmap(void);

/** Is a pid valid?
 */
int pid_valid(pid_t);

/** Send a signal to a process.
 */
int signal_post(pid_t, int);

struct dpc
{
  struct dpc * next;                    // Next in the list.
  int state;                            // DPC state.
  void (*fn)(void *);                   // Defered procedure...
  void *arg;                            // ...and its argument.
};

/** Schedule a defered procedure call.
 */
void sched_dpc(struct dpc *, void (*)(void *), void *);

/** Get and set the per-process brk pointer.
 */
char *get_brk(pid_t pid);
void set_brk(pid_t pid, char *brk);

#endif
