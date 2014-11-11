/* A simple bare metal main.
 */

#include <stdio.h>
#include "command.h"

int main(int argc, char **argv)
{
  printf("%s started. Type \"help\" for a list of commands.\n", argv[0]);

  // Enter the kernel command processor.
  do_commands(argv[0]);
}

