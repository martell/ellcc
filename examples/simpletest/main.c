/* A simple bare metal main.
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/mount.h>
#include "command.h"

int main(int argc, char **argv)
{
  printf("%s started. Type \"help\" for a list of commands.\n", argv[0]);

#if 1
  int s = mount("", "/", "devfs", 0, NULL);
  if (s) {
    printf("mount failed: %s\n", strerror(errno));
  }
#endif

  // Enter the kernel command processor.
  do_commands(argv[0]);
}

