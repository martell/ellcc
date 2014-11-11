/* A simple bare metal main.
 */

#include <stdio.h>
#include "command.h"
#include "file.h"

int main(int argc, char **argv)
{
  // Create a file descriptor binding.
  static const fileops_t fileops = {
    fnullop_read, fnullop_write, fnullop_ioctl, fnullop_fcntl,
    fnullop_poll, fnullop_stat, fnullop_close
  };
  static fdset_t fdset = NULL;
  int s;

  int fd0 = __elk_fdset_add(&fdset, FTYPE_MISC, &fileops);
  if (fd0 < 0) {
    printf("__elk_fdset_add error: %s\n", strerror(-fd0));
  } else {
    printf("__elk_fdset_add returned: %d\n", fd0);
  }

  int fd1 = __elk_fdset_add(&fdset, FTYPE_MISC, &fileops);
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

  s = __elk_fdset_remove(&fdset, fd0);
  if (s < 0) {
    printf("__elk_fdset_remove error: %s\n", strerror(-s));
  } else {
    printf("__elk_fdset_remove returned: %d\n", s);
  }
}

