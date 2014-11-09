/** Semaphore implementaion.
 */

#include <errno.h>
#include <errno.h>
#include <semaphore.h>
#include "kernel.h"
#include "scheduler.h"
#include "timer.h"

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

  sem->lock = (__elk_lock)LOCK_INITIALIZER;
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
      __elk_thread *me = __elk_thread_self();
      me->next = sem->waiters;
      sem->waiters = me;
      __elk_lock_release(&sem->lock);
      s = __elk_change_state(0, SEMWAIT);
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
  __elk_thread *thread = (__elk_thread *)arg2;
  __elk_lock_aquire(&sem->lock);
  __elk_thread *p, *q;
  for (p = sem->waiters, q = NULL; p; q = p, p = p->next) {
    if (p == thread) {
      // This thread timed out.
      if (q) {
        q->next = p->next;
      } else {
        sem->waiters = p->next;
      }
      p->next = NULL;
      __elk_context_set_return(p->saved_ctx, -ETIMEDOUT);
      __elk_schedule(p);
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
      long long now = timer_get_realtime();
      if (now > when) {
        // Already expired.
        __elk_lock_release(&sem->lock);
        errno = ETIMEDOUT;
        s = -1;
      } else {
        __elk_thread *me = __elk_thread_self();
        me->next = sem->waiters;
        sem->waiters = me;
        __elk_lock_release(&sem->lock);
        when -= timer_get_realtime_offset();
        void *t = timer_wake_at(when, callback,
                                (void *)sem, (void *)me);
        s = __elk_change_state(0, SEMTMO);
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
  __elk_thread *list = NULL;
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
    __elk_schedule(list);
  }
  return s;
}
