/** Handle thread timeout callbacks and wakeups.
 */
#include <syscalls.h>           // For syscall numbers.
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <kernel.h>
#include <timer.h>
#include <scheduler.h>
#include <command.h>

// Make the time syscalls commands a loadable feature.
FEATURE(timer, timer)

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

  __elk_timer_wake_at(when, NULL, 0, 0);
  return 0;
}

static int sys_nanosleep(const struct timespec *req, struct timespec *rem)
{
  return sys_clock_nanosleep(CLOCK_MONOTONIC, 0, req, rem);
}

static int dateCommand(int argc, char **argv)
{
  if (argc <= 0) {
    printf("show/set the system clock.\n");
    if (argc < 0) {
      printf("%sThe argument used to set the time, if given,\n", *argv);
      printf("%sis in the form [MMDDhhmm[[CC]YY][.ss]].\n", *argv);
    }
    return COMMAND_OK;
  }

  if (argc > 1) {
    // Set the time.
    struct tm tm;
    time_t t = time(NULL);
    localtime_r(&t, &tm);

    char *p = argv[1];          // Point to the date string.

    // Find seconds.
    char *s = strchr(p, '.');
    if (s) {
      // Seconds here. Terminate p and point to them.
      *s++ = '\0';
    }

    int left = strlen(p);       // Number of characters in non-second string.

#define TODEC(v) if (*(p)) { v = 0; v += *p - '0'; ++p; --left; } \
             if (*(p)) { v *= 10; v += *p - '0'; ++p; --left; }

    TODEC(tm.tm_mon)
    if (tm.tm_mon < 1 || tm.tm_mon > 12) {
      printf("invalid month: %d\n", tm.tm_mon);
      return COMMAND_ERROR;
    }
    --tm.tm_mon;    // In the range of 0 .. 11.

    TODEC(tm.tm_mday)
    if (tm.tm_mday < 1 || tm.tm_mday > 31) {
      printf("invalid day of month: %d\n", tm.tm_mday);
      return COMMAND_ERROR;
    }

    TODEC(tm.tm_hour)
    if (tm.tm_hour < 0 || tm.tm_hour > 23) {
      printf("invalid hour: %d\n", tm.tm_hour);
      return COMMAND_ERROR;
    }

    TODEC(tm.tm_min)
    if (tm.tm_min < 0 || tm.tm_min > 59) {
      printf("invalid minute: %d\n", tm.tm_min);
      return COMMAND_ERROR;
    }

    if (left >= 2) {
      int year = 0;
      if (left >= 4) {
        // Have a four digit year.
        TODEC(year)
        year = year * 100;
      } else {
        year = 1900;
      }

      int tens = 0;
      TODEC(tens)
      year += tens;
      if (year < 1900) {
        printf("invalid year: %d\n", year);
        return COMMAND_ERROR;
      }

      tm.tm_year = year - 1900;
    }

    if (s) {
      // Have seconds.
      p = s;
      TODEC(tm.tm_sec);
      if (tm.tm_sec < 0 || tm.tm_sec > 59) {
        printf("invalid seconds: %d\n", tm.tm_sec);
        return COMMAND_ERROR;
      }
    }

    time_t sec = mktime(&tm);
    if (sec == (time_t)-1) {
      printf("mktime failed %s\n", asctime(&tm));
      return COMMAND_ERROR;
    }
    struct timeval tv = { sec, 0 };
    return settimeofday(&tv, NULL) == 0 ? COMMAND_OK : COMMAND_ERROR;
  }

  time_t t = time(NULL);
  char date[26];
  fputs(ctime_r(&t, date), stdout);
  return COMMAND_OK;
}

/* Time a command.
 */
static int timeCommand(int argc, char **argv)
{
  if (argc <= 0) {
    printf("time the specified command with arguments.\n");
    return COMMAND_OK;
  }

  if (argc < 2) {
    printf("no command specified\n");
    return COMMAND_ERROR;
  }

  long long t = __elk_timer_get_monotonic();
  int s = run_command(argc - 1, argv + 1);
  t = __elk_timer_get_monotonic() - t;
  printf("elapsed time: %ld.%09ld sec\n", (long)(t / 1000000000),
         (long)(t % 1000000000));
  return s;
}

/* Sleep for a time period.
 */
static int sleepCommand(int argc, char **argv)
{
  if (argc <= 0) {
    printf("sleep for a time period.\n");
    return COMMAND_OK;
  }

  if (argc < 2) {
    printf("no period specified\n");
    return COMMAND_ERROR;
  }

  long sec = 0;
  long nsec = 0;
  char *p = strchr(argv[1], '.');
  if (p) {
    // Have a decimal point.
    // (Remember, no floating point in the kernel for now.)
    char *end;
    nsec = strtol(p + 1, &end, 10);
    int digits = end - (p + 1) - 1;
    if (digits > 8) digits = 8;
    static const int powers[9] = { 100000000, 10000000, 1000000, 100000,
                                   10000, 1000, 100, 10, 1 };
    nsec *= powers[digits];
    *p = '\0';
  }
  sec = strtol(argv[1], NULL, 10);

  struct timespec ts = { sec, nsec };
  nanosleep(&ts, NULL);
  return COMMAND_ERROR;
}

CONSTRUCTOR()
{
  // Set up timer handling.

  // Set up time related system calls.
  __elk_set_syscall(SYS_clock_getres, sys_clock_getres);
  __elk_set_syscall(SYS_clock_gettime, sys_clock_gettime);
  __elk_set_syscall(SYS_clock_settime, sys_clock_settime);
  __elk_set_syscall(SYS_clock_nanosleep, sys_clock_nanosleep);
  __elk_set_syscall(SYS_nanosleep, sys_nanosleep);
  __elk_set_syscall(SYS_settimeofday, sys_settimeofday);
  __elk_set_syscall(SYS_gettimeofday, sys_gettimeofday);

  command_insert("date", dateCommand);
  command_insert("time", timeCommand);
  command_insert("sleep", sleepCommand);
}
