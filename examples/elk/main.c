/* ELK running as a VM enabled OS.
 */
#include <sys/cdefs.h>
#include <stdio.h>

#include "command.h"

int main(int argc, char **argv)
{
  setprogname("elk");
  printf("%s started. Type \"help\" for a list of commands.\n", getprogname());
  // Enter the kernel command processor.
  do_commands(getprogname());
}

