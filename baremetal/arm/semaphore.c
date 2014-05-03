/** A simple semaphore implementaion.
 */

#include <errno.h>
#include "kernel.h"
#include "scheduler.h"
#include "semaphore.h"

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
        return -ENOSYS;
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
        s = -EOVERFLOW;
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

