/** Some handy time related commands.
 */
#include <syscalls.h>           // For syscall numbers.
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "kernel.h"
#include "timer.h"
#include "thread.h"
#include "command.h"

// Make the time syscalls commands a loadable feature.
FEATURE(time_commands)

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

  long long t = timer_get_monotonic();
  int s = run_command(argc - 1, argv + 1);
  t = timer_get_monotonic() - t;
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

/** Create a section heading for the help command.
 */
static int sectionCommand(int argc, char **argv)
{
  if (argc <= 0 ) {
    printf("Time Commands:\n");
  }
  return COMMAND_OK;
}

ELK_PRECONSTRUCTOR()
{
  command_insert(NULL, sectionCommand);
  command_insert("date", dateCommand);
  command_insert("time", timeCommand);
  command_insert("sleep", sleepCommand);
}
