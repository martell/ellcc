#include <bits/syscall.h>       // For syscall numbers.
#include <time.h>
#include <errno.h>
#include "kernel.h"
#include "timer.h"

/* Define the system clocks.
 */
Lock realtime_lock;
static struct timespec realtime;
Lock monotonic_lock;
static struct timespec monotonic;

static int sys_clock_getres(clockid_t clock, struct timespec *res)
{
    if (res) {
        VALIDATE_ADDRESS(res, sizeof(*res), VALID_WR);
    }

    res->tv_sec = 0;
    res->tv_nsec = timer_getres();
    return 0;
}

static int sys_clock_gettime(clockid_t clock, struct timespec *tp)
{
    VALIDATE_ADDRESS(tp, sizeof(*tp), VALID_WR);

    switch (clock) {
    case CLOCK_REALTIME:
        lock_aquire(&realtime_lock);
        *tp = realtime;
        lock_release(&realtime_lock);
        break;

    case CLOCK_MONOTONIC:
        lock_aquire(&monotonic_lock);
        *tp = monotonic;
        lock_release(&monotonic_lock);
        break;

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
    switch (clock) {
    case CLOCK_REALTIME:
        // RICH: Permissions.
        lock_aquire(&realtime_lock);
        realtime = *tp;
        lock_release(&realtime_lock);
        break;

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

    if (req->tv_nsec < 0 || req->tv_nsec > 999999999 ||
        req->tv_sec < 0) {
        return -EINVAL;
    }

    return -ENOSYS;
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

