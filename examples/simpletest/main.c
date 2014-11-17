/* A simple bare metal main.
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include "command.h"

int main(int argc, char **argv)
{
  printf("%s started. Type \"help\" for a list of commands.\n", argv[0]);

#if 1
  int s;
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
#endif

  // Enter the kernel command processor.
  do_commands(argv[0]);
}

