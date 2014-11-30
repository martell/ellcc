/* ELK running as a boot strap program.
 */
#include <sys/cdefs.h>
#include <stdio.h>

#include "command.h"

int main(int argc, char **argv)
{
  setprogname("elkboot");
  printf("%s started. Type \"help\" for a list of commands.\n", getprogname());
  // Enter the kernel command processor.
  do_commands(getprogname());
}

