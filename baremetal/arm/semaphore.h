/** A simple semaphore implementaion.
 */

#ifndef _semaphore_h
#define _semaphore_h

#include "kernel.h"

typedef struct {
    Lock lock;
    unsigned count;
    Thread *waiters;
} sem_t;

/** Initialize a semaphore.
 * @param sem A pointer to the semaphore.
 * @param pshared != 0 if this semaphore is shared among processes.
 *                     This is not implemented.
 * @param value The initial semaphore value.
 */
int sem_init(sem_t *sem, int pshared, unsigned int value);

/** Wait on a semaphore.
 * @param sem A pointer to the semaphore.
 */
int sem_wait(sem_t *sem);

/** Unlock a semaphore.
 * @param sem A pointer to the semaphore.
 */
int sem_post(sem_t *sem);

#endif // _semaphore_h
