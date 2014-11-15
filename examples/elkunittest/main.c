/* A simple bare metal main.
 */

#include <stdio.h>
#include <pthread.h>
#include "command.h"
#include "file.h"
#include "device.h"

int main(int argc, char **argv)
{
  // Create a file descriptor binding.
  static const fileops_t fileops = {
    fnullop_read, fnullop_write, fnullop_ioctl, fnullop_fcntl,
    fnullop_poll, fnullop_stat, fnullop_close
  };
  static fdset_t fdset = { .mutex = PTHREAD_MUTEX_INITIALIZER };
  int s;

  int fd0 = __elk_fdset_add(&fdset, FTYPE_MISC, &fileops, NULL);
  if (fd0 < 0) {
    printf("__elk_fdset_add error: %s\n", strerror(-fd0));
  } else {
    printf("__elk_fdset_add returned: %d\n", fd0);
  }

  int fd1 = __elk_fdset_add(&fdset, FTYPE_MISC, &fileops, NULL);
  if (fd1 < 0) {
    printf("__elk_fdset_add error: %s\n", strerror(-fd1));
  } else {
    printf("__elk_fdset_add returned: %d\n", fd1);
  }

  s = __elk_fdset_remove(&fdset, 3);
  if (s < 0) {
    printf("__elk_fdset_remove error: %s\n", strerror(-s));
  } else {
    printf("__elk_fdset_remove returned: %d\n", s);
  }

  s = __elk_fdset_dup(&fdset, fd0);
  if (s < 0) {
    printf("__elk_fdset_dup error: %s\n", strerror(-s));
  } else {
    printf("__elk_fdset_dup returned: %d\n", s);
  }

  s = __elk_fdset_remove(&fdset, fd0);
  if (s < 0) {
    printf("__elk_fdset_remove error: %s\n", strerror(-s));
  } else {
    printf("__elk_fdset_remove returned: %d\n", s);
  }

  __elk_fdset_release(&fdset);
  if (fdset.fds != NULL) {
    printf("__elk_fdset_release failed.\n");
  }

#if RICH
  struct driver drv = {};
  device_t devp = __elk_device_create(&drv, "/dev/tty", D_CHR);

  do_commands(argv[0]);
#endif
}
