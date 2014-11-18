/* A simple bare metal main.
 */

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
  printf("%s started. Type \"help\" for a list of commands.\n", argv[0]);

#if 1
  int s, fd;
  s = mount("", "/", "ramfs", 0, NULL);
  if (s) {
    printf("ramfs mount failed: %s\n", strerror(errno));
  }
  fd = open(".././.././foo", O_CREAT|O_WRONLY, 0777);
  if (fd < 0) {
    printf("open(/foo) failed: %s\n", strerror(errno));
  }
  s = write(fd, "hello world\n", sizeof("hello world\n"));
  if (s < 0) {
    printf("write failed: %s\n", strerror(errno));
  } else {
    printf("write wrote: %d\n", s);
  }
  close(fd);
  fd = open("/foo", O_RDONLY);
  if (fd < 0) {
    printf("open(/foo) failed: %s\n", strerror(errno));
  }
  char buffer[100];
  s = read(fd, buffer, 100);
  if (s < 0) {
    printf("read failed: %s\n", strerror(errno));
  } else {
    printf("read read: %d\n", s);
  }
  printf("I read %s", buffer);

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

