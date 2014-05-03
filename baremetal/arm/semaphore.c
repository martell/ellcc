/** A simple semaphore implementaion.
 */

#include <errno.h>
#include <errno.h>
#include "kernel.h"
#include "scheduler.h"
#include "semaphore.h"
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
    lock_aquire(&sem->lock);
    if (sem->count) {
        --sem->count;
        lock_release(&sem->lock);
    } else {
        Thread *me = __get_self();
        me->next = sem->waiters;
        sem->waiters = me;
        lock_release(&sem->lock);
        change_state(SEMWAIT);
    }
    return 0;
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

static void callback(intptr_t arg1, intptr_t arg2)
{
    // RICH: cancel wait.
}

/** Wait on a semaphore with a timeout.
 * @param sem A pointer to the semaphore.
 * @param abs_timeout The timeout.
 */
int sem_timedwait(sem_t *sem, struct timespec *abs_timeout)
{
    lock_aquire(&sem->lock);
    if (sem->count) {
        --sem->count;
        lock_release(&sem->lock);
    } else {
        long long when;
        when = abs_timeout->tv_sec * 1000000000LL + abs_timeout->tv_nsec;
        long long now = timer_get_realtime();
        if (now > when) {
            // Already expired.
            errno = ETIMEDOUT;
            lock_release(&sem->lock);
        } else {
            Thread *me = __get_self();
            me->next = sem->waiters;
            sem->waiters = me;
            lock_release(&sem->lock);
            when -= timer_get_realtime_offset();
            timer_wake_at(when, callback, (intptr_t) sem, (intptr_t) me);
            change_state(SEMWAIT);
        }
    }
    return 0;
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

