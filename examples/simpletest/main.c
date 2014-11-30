/* A simple bare metal main.
 */

#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "command.h"

int main(int argc, char **argv)
{
  int s, fd;
  s = mount("", "/", "ramfs", 0, NULL);
  if (s) {
    printf("ramfs mount failed: %s\n", strerror(errno));
  }
  s = mkdir("/dev", S_IRWXU);
  if (s) {
    printf("/dev mkdir failed: %s\n", strerror(errno));
  }
  s = mount("", "/dev", "devfs", 0, NULL);
  if (s) {
    printf("devfs mount failed: %s\n", strerror(errno));
  }

  fd = open("/dev/tty", O_RDWR);
  assert(fd >= 0);
  dup2(fd, 0);
  dup2(fd, 1);
  dup2(fd, 2);
  if (fd != 0)
      close(fd);

  printf("%s started. Type \"help\" for a list of commands.\n", argv[1]);
  // Enter the kernel command processor.
  do_commands(argv[0]);
}

