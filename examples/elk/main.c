/* ELK running as a VM enabled OS.
 */
#include <sys/cdefs.h>
#include <stdio.h>

#include "command.h"


int main(int argc, char **argv)
{
#if 0
  // LWIP testing.
  void tcpip_init(void *, void *);
  tcpip_init(0, 0);
#endif
  setprogname("elk");
  printf("%s started. Type \"help\" for a list of commands.\n", getprogname());
  // Enter the kernel command processor.
  do_commands(getprogname());
}

