/* ELK running as a VM enabled OS.
 */
#include <sys/cdefs.h>
#include <stdio.h>

#include "command.h"


#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
int main(int argc, char **argv)
{
  int s = socket(AF_UNIX, SOCK_STREAM|SOCK_NONBLOCK, 0);
  if (s < 0) {
    printf("socket() failed: %s\n", strerror(errno));
    exit(1);
  }
  struct sockaddr_un addr;
  strcpy(addr.sun_path, "socket");
  addr.sun_family = AF_UNIX;
  s = bind(s, (const struct sockaddr *)&addr, sizeof(addr.sun_family) + strlen(addr.sun_path) + 1);
  if (s != 0) {
    printf("bind() failed: %s\n", strerror(errno));
  }

  setprogname("elk");
  printf("%s started. Type \"help\" for a list of commands.\n", getprogname());
  // Enter the kernel command processor.
  do_commands(getprogname());
}

