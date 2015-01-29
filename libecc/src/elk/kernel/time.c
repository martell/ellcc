/** Time releated system calls.
 */
#include <syscalls.h>           // For syscall numbers.
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

#include "kernel.h"
#include "timer.h"
#include "thread.h"
#include "crt1.h"

// Make the time syscalls commands a loadable feature.
FEATURE(time)

static int sys_clock_getres(clockid_t clock, struct timespec *res)
{
  switch (clock) {
  case CLOCK_MONOTONIC:
  case CLOCK_REALTIME:
    if (res) {
      struct timespec val;
      val.tv_sec = 0;
      val.tv_nsec = timer_getres();
      return copyout(&val, res, sizeof(*res));
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
  struct timespec val;

  switch (clock) {
  case CLOCK_REALTIME: {
    long long realtime = timer_get_realtime();
    val.tv_sec = realtime / 1000000000;
    val.tv_nsec = realtime % 1000000000;
    break;
  }

  case CLOCK_MONOTONIC: {
    long long monotonic = timer_get_monotonic();
    val.tv_sec = monotonic / 1000000000;
    val.tv_nsec = monotonic % 1000000000;
    break;
  }

  case CLOCK_PROCESS_CPUTIME_ID:
  case CLOCK_THREAD_CPUTIME_ID:
  default:
    return -EINVAL;
  }

  return copyout(&val, tp, sizeof(*tp));
}

static int sys_clock_settime(clockid_t clock, const struct timespec *tp)
{
  struct timespec val;
  int s = copyin(tp, &val, sizeof(val));
  if (s != 0)
    return s;

  if (val.tv_nsec < 0 || val.tv_nsec >= 1000000000) {
    return -EINVAL;
  }

  switch (clock) {
  case CLOCK_REALTIME: {
    // RICH: Permissions.
    long long realtime = val.tv_sec * 1000000000LL + val.tv_nsec;
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

static int sys_settimeofday(struct timeval *tv)
{
  struct timeval val;
  int s = copyin(tv, &val, sizeof(val));
  if (s != 0)
    return s;
  struct timespec ts;

  ts.tv_sec = val.tv_sec;
  ts.tv_nsec = val.tv_usec * 1000;
  return sys_clock_settime(CLOCK_REALTIME, &ts);
}

static int sys_gettimeofday(struct timeval *tv)
{
  struct timespec ts;
  int s = sys_clock_gettime(CLOCK_REALTIME, &ts);
  if (s < 0) {
    return s;
  }

  struct timeval val;
  val.tv_sec = ts.tv_sec;
  val.tv_usec = ts.tv_nsec / 1000;
  return copyout(&val, tv, sizeof(*tv));
}

static int sys_clock_nanosleep(clockid_t clock, int flags,
                               const struct timespec *req,
                               struct timespec *rem)
{
  struct timespec val;
  int s = copyin(req, &val, sizeof(val));
  if (s != 0)
    return s;

  if (val.tv_nsec < 0 || val.tv_nsec >= 1000000000 || val.tv_sec < 0) {
    return -EINVAL;
  }

  // Get the desired time or delta.
  long long when = val.tv_sec * 1000000000LL + val.tv_nsec;

  // Get the current time.
  long long now;
  switch (clock) {
  case CLOCK_REALTIME: {
    now = timer_get_realtime();
    // RICH: need to adjust.
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

  if (timer_wake_at(when, NULL, 0, 0, 0) == NULL && errno == EINTR) {
    // RICH: Set rem.
  }
  return 0;
}

static int sys_nanosleep(const struct timespec *req, struct timespec *rem)
{
  return sys_clock_nanosleep(CLOCK_MONOTONIC, 0, req, rem);
}

ELK_PRECONSTRUCTOR()
{
  // Set up time related system calls.
  SYSCALL(clock_getres);
  SYSCALL(clock_gettime);
  SYSCALL(clock_settime);
  SYSCALL(clock_nanosleep);
  SYSCALL(nanosleep);
  SYSCALL(settimeofday);
  SYSCALL(gettimeofday);
}
