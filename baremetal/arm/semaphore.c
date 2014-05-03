/** A simple semaphore implementaion.
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
int sem_init(sem_t *sem, int pshared, unsigned int value)
{
    if (pshared) {
        // Not supported.
        errno = ENOSYS;
        return -1;
    }

    sem->lock = (Lock)LOCK_INITIALIZER;
    sem->count = value;
    sem->waiters = NULL;
    return 0;
}

/** Wait on a semaphore.
 * @param sem A pointer to the semaphore.
 */
int sem_wait(sem_t *sem)
{
    int s = 0;
    lock_aquire(&sem->lock);
    for ( ;; ) {
        if (sem->count) {
            --sem->count;
            lock_release(&sem->lock);
            break;
        } else {
            Thread *me = __get_self();
            me->next = sem->waiters;
            sem->waiters = me;
            lock_release(&sem->lock);
            s = change_state(SEMWAIT);
            if (s < 0) {
                errno = -s;
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
int sem_try_wait(sem_t *sem)
{
    lock_aquire(&sem->lock);
    int s = 0;
    if (sem->count) {
        --sem->count;
    } else {
        // Would have to wait.
        errno = EAGAIN;
        s = -1;
    }
    lock_release(&sem->lock);
    return s;
}

/** This callback occurs when a sem_timedwait timeout expires.
 */
static void callback(intptr_t arg1, intptr_t arg2)
{
    sem_t *sem = (sem_t *)arg1;
    Thread *thread = (Thread *)arg2;
    lock_aquire(&sem->lock);
    Thread *p, *q;
    for (p = sem->waiters, q = NULL; p; q = p, p = p->next) {
        if (p == thread) {
            // This thread timed out.
            if (q) {
                q->next = p->next;
            } else {
                sem->waiters = p->next;
            }
            p->next = NULL;
            context_set_return(p->saved_sp, -ETIMEDOUT);
            schedule(p);
        }
    }
    lock_release(&sem->lock);
}

/** Wait on a semaphore with a timeout.
 * @param sem A pointer to the semaphore.
 * @param abs_timeout The timeout.
 */
int sem_timedwait(sem_t *sem, struct timespec *abs_timeout)
{
    int s = 0;
    lock_aquire(&sem->lock);
    for ( ;; ) {
        if (sem->count) {
            --sem->count;
            lock_release(&sem->lock);
            break;
        } else {
            long long when;
            when = abs_timeout->tv_sec * 1000000000LL + abs_timeout->tv_nsec;
            long long now = timer_get_realtime();
            if (now > when) {
                // Already expired.
                lock_release(&sem->lock);
                errno = ETIMEDOUT;
                s = -1;
            } else {
                Thread *me = __get_self();
                me->next = sem->waiters;
                sem->waiters = me;
                lock_release(&sem->lock);
                when -= timer_get_realtime_offset();
                void *t = timer_wake_at(when, callback,
                                        (intptr_t) sem, (intptr_t) me);
                context_set_return(me->saved_sp, 0);
                s = change_state(SEMTMO);
                timer_cancel_wake_at(t);
                if (s < 0) {
                    errno = -s;
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
int sem_post(sem_t *sem)
{
    int s = 0;
    Thread *list = NULL;
    lock_aquire(&sem->lock);
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
    lock_release(&sem->lock);
    if (list) {
        schedule(list);
    }
    return s;
}

