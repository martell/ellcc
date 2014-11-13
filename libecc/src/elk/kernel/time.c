/** Time releated system calls.
 */
#include <syscalls.h>           // For syscall numbers.
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <kernel.h>
#include <timer.h>
#include <thread.h>

// Make the time syscalls commands a loadable feature.
FEATURE(time, time)

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
      res->tv_nsec = __elk_timer_getres();
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
    long long realtime = __elk_timer_get_realtime();
    tp->tv_sec = realtime / 1000000000;
    tp->tv_nsec = realtime % 1000000000;
    break;
  }

  case CLOCK_MONOTONIC: {
    long long monotonic = __elk_timer_get_monotonic();
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
    __elk_timer_set_realtime(realtime);
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

static int sys_settimeofday(struct timeval *tv)
{
  VALIDATE_ADDRESS(tv, sizeof(*tv), VALID_WR);
  struct timespec ts;
  ts.tv_sec = tv->tv_sec;
  ts.tv_nsec = tv->tv_usec * 1000;
  return sys_clock_settime(CLOCK_REALTIME, &ts);
}

static int sys_gettimeofday(struct timeval *tv)
{
  VALIDATE_ADDRESS(tv, sizeof(*tv), VALID_RD);
  struct timespec ts;
  int s = sys_clock_gettime(CLOCK_REALTIME, &ts);
  if (s < 0) {
    return s;
  }

  tv->tv_sec = ts.tv_sec;
  tv->tv_usec = ts.tv_nsec / 1000;
  return 0;
}

static int sys_clock_nanosleep(clockid_t clock, int flags,
                               const struct timespec *req,
                               struct timespec *rem)
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
    now = __elk_timer_get_realtime();
    // RICH: need to adjust.
    break;
  }

  case CLOCK_MONOTONIC: {
    now = __elk_timer_get_monotonic();
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

  __elk_timer_wake_at(when, NULL, 0, 0, 0);
  // RICH: Check for interrupted call, set rem.
  return 0;
}

static int sys_nanosleep(const struct timespec *req, struct timespec *rem)
{
  return sys_clock_nanosleep(CLOCK_MONOTONIC, 0, req, rem);
}

ELK_CONSTRUCTOR()
{
  // Set up time related system calls.
  __elk_set_syscall(SYS_clock_getres, sys_clock_getres);
  __elk_set_syscall(SYS_clock_gettime, sys_clock_gettime);
  __elk_set_syscall(SYS_clock_settime, sys_clock_settime);
  __elk_set_syscall(SYS_clock_nanosleep, sys_clock_nanosleep);
  __elk_set_syscall(SYS_nanosleep, sys_nanosleep);
  __elk_set_syscall(SYS_settimeofday, sys_settimeofday);
  __elk_set_syscall(SYS_gettimeofday, sys_gettimeofday);
}
