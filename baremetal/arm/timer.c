#include <bits/syscall.h>       // For syscall numbers.
#include <time.h>
#include <errno.h>
#include <stdlib.h>
#include "kernel.h"
#include "timer.h"
#include "scheduler.h"

/** The timeout list.
 * This list is kept in order of expire times.
 */

struct timeout {
    struct timeout *next;
    long long when;             // When the timeout will expire.
    Thread *waiter;             // The waiting thread.
};

static Lock timeout_lock;
static struct timeout *timeouts;

/** Make an entry in the sleeping list and sleep.
 */
static int sleep_until(long long when)
{
    struct timeout *tmo = malloc(sizeof(struct timeout));
    tmo->next = NULL;
    tmo->when = when;
    tmo->waiter = __get_self();

    lock_aquire(&timeout_lock);
    // Search the list.
    if (timeouts == NULL) {
        timeouts = tmo;
    } else {
        struct timeout *p, *q;
        for (p = timeouts, q = NULL; p && p->when > when; q = p, p = p->next)
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
    timer_start(timeouts->when);
    lock_release(&timeout_lock);
    int s = 0;
    do {
        Message msg = get_message(NULL);    // Wait for a message.
        switch (msg.code) {
        case MSG_TIMEOUT:                   // The timeout occured.
            return 0;

        default:                            // Unexpected message.
            break;
        }
    } while (s == 0);

    return s;
}

/** Timer expired handler.
 * This function is called in an interrupt context.
 */
long long timer_expired(long long when)
{
    lock_aquire(&timeout_lock);
    Thread *ready = NULL;
    Thread *next = NULL;
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

        free(tmo);
    }

    if (timeouts == NULL) {
        when = 0;
    } else {
        when = timeouts->when;
    }

    lock_release(&timeout_lock);

    if (ready) {
        // Start one or more threads.

        while (ready) {
            Thread *next = ready;
            ready = ready->next;
            next->next = NULL;
            send_message(&next->queue, (Message){ MSG_TIMEOUT });
        }
    }

    return when;        // Schedule the next timeout, if any.
}

static int sys_clock_getres(clockid_t clock, struct timespec *res)
{
    if (res) {
        VALIDATE_ADDRESS(res, sizeof(*res), VALID_WR);
    }

    switch (clock) {
    case CLOCK_MONOTONIC:
    case CLOCK_REALTIME:
        if (res) {
            res->tv_sec = 0;
            res->tv_nsec = timer_getres();
        }
        break;

    case CLOCK_PROCESS_CPUTIME_ID:
    case CLOCK_THREAD_CPUTIME_ID:
    default:
        return -EINVAL;
    }

    return 0;
}

static int sys_clock_gettime(clockid_t clock, struct timespec *tp)
{
    VALIDATE_ADDRESS(tp, sizeof(*tp), VALID_WR);

    switch (clock) {
    case CLOCK_REALTIME: {
        long long realtime = timer_get_realtime();
        tp->tv_sec = realtime / 1000000000;
        tp->tv_nsec = realtime % 1000000000;
        break;
    }

    case CLOCK_MONOTONIC: {
        long long monotonic = timer_get_monotonic();
        tp->tv_sec = monotonic / 1000000000;
        tp->tv_nsec = monotonic % 1000000000;
        break;
    }

    case CLOCK_PROCESS_CPUTIME_ID:
    case CLOCK_THREAD_CPUTIME_ID:
    default:
        return -EINVAL;
    }

    return 0;
}

static int sys_clock_settime(clockid_t clock, const struct timespec *tp)
{
    VALIDATE_ADDRESS(tp, sizeof(*tp), VALID_RD);

    if (tp->tv_nsec < 0 || tp->tv_nsec >= 1000000000) {
        return -EINVAL;
    }

    switch (clock) {
    case CLOCK_REALTIME: {
        // RICH: Permissions.
        long long realtime = tp->tv_sec * 1000000000LL + tp->tv_nsec;
        timer_set_realtime(realtime);
        break;
    }

    case CLOCK_MONOTONIC:
        // Can't change this clock.
        return -EPERM;

    case CLOCK_PROCESS_CPUTIME_ID:
    case CLOCK_THREAD_CPUTIME_ID:
    default:
        return -EINVAL;
    }

    return 0;
}

static int sys_clock_nanosleep(clockid_t clock, int flags,
                               const struct timespec *req, struct timespec *rem)
{
    VALIDATE_ADDRESS(req, sizeof(*req), VALID_RD);
    if (rem) {
        VALIDATE_ADDRESS(rem, sizeof(*rem), VALID_WR);
    }

    if (req->tv_nsec < 0 || req->tv_nsec >= 1000000000 ||
        req->tv_sec < 0) {
        return -EINVAL;
    }

    // Get the desired time or delta.
    long long when = req->tv_sec * 1000000000LL + req->tv_nsec;

    // Get the current time.
    long long now;
    switch (clock) {
    case CLOCK_REALTIME: {
        now = timer_get_realtime();
        break;
    }

    case CLOCK_MONOTONIC: {
        now = timer_get_monotonic();
        break;
    }

    case CLOCK_PROCESS_CPUTIME_ID:
    case CLOCK_THREAD_CPUTIME_ID:
    default:
        return -EINVAL;
    }

    if (!(flags & TIMER_ABSTIME)) {
        // This is a relative time, make it absolute.
        when += now;
    }

    if (when <= now) {
        // No sleep needed.
        return 0;
    }

    return sleep_until(when);
}

static int sys_nanosleep(const struct timespec *req, struct timespec *rem)
{

    return sys_clock_nanosleep(CLOCK_MONOTONIC, 0, req, rem);
}

static void init(void)
    __attribute__((__constructor__, __used__));

static void init(void)
{
    // Set up timer handling.

    // Set up time related system calls.
    __set_syscall(SYS_clock_getres, sys_clock_getres);
    __set_syscall(SYS_clock_gettime, sys_clock_gettime);
    __set_syscall(SYS_clock_settime, sys_clock_settime);
    __set_syscall(SYS_clock_nanosleep, sys_clock_nanosleep);
    __set_syscall(SYS_nanosleep, sys_nanosleep);
}

