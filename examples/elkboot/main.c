/* ELK running as a boot strap program.
 */
#include <sys/cdefs.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>

#include "command.h"

int main(int argc, char **argv)
{
  setprogname("elkboot");
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

  printf("%s started. Type \"help\" for a list of commands.\n", getprogname());
  // Enter the kernel command processor.
  do_commands(getprogname());
}

