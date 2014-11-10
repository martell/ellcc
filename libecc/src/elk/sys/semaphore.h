/** Semaphore interface.
 */

#ifndef _semaphore_h
#define _semaphore_h

#include <time.h>
#include "kernel.h"

typedef struct {
  lock_t lock;
  unsigned count;
  struct thread *waiters;
} __elk_sem_t;

/** Initialize a semaphore.
 * @param sem A pointer to the semaphore.
 * @param pshared != 0 if this semaphore is shared among processes.
 *                     This is not implemented.
 * @param value The initial semaphore value.
 */
int __elk_sem_init(__elk_sem_t *sem, int pshared, unsigned int value);

/** Wait on a semaphore.
 * @param sem A pointer to the semaphore.
 */
int __elk_sem_wait(__elk_sem_t *sem);

/** Try to take a semaphore.
 * @param sem A pointer to the semaphore.
 */
int __elk_sem_try_wait(__elk_sem_t *sem);

/** Wait on a semaphore with a timeout.
 * @param sem A pointer to the semaphore.
 * @param abs_timeout The timeout based on CLOCK_REALTIME.
 */
int __elk_sem_timedwait(__elk_sem_t *sem, struct timespec *abs_timeout);

/** Unlock a semaphore.
 * @param sem A pointer to the semaphore.
 */
int __elk_sem_post(__elk_sem_t *sem);

#endif // _semaphore_h
