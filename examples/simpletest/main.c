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

#if 0
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
    printf("I read %s", buffer);
  }
  s = close(fd);
  if (s < 0) {
    printf("close failed: %s\n", strerror(errno));
  }

  char *p = getcwd(buffer, 100);
  if (p) {
    printf("getcwd got: %s\n", buffer);
  } else {
    printf("getcwd failed: %s\n", strerror(errno));
  }

  s = chdir("/");
  if (s) {
    printf("chdir /dev failed: %s\n", strerror(errno));
  }

  p = getcwd(buffer, 100);
  if (p) {
    printf("getcwd got: %s\n", buffer);
  } else {
    printf("getcwd failed: %s\n", strerror(errno));
  }

  s = mkdir("/fee", S_IRWXU);
  s = chdir("/fee");
  if (s) {
    printf("chdir /fee failed: %s\n", strerror(errno));
  }


  p = getcwd(buffer, 100);
  if (p) {
    printf("getcwd got: %s\n", buffer);
  } else {
    printf("getcwd failed: %s\n", strerror(errno));
  }
  s = chdir("..//dev");
  if (s) {
    printf("chdir /dev failed: %s\n", strerror(errno));
  }

  p = getcwd(buffer, 100);
  if (p) {
    printf("getcwd got: %s\n", buffer);
  } else {
    printf("getcwd failed: %s\n", strerror(errno));
  }

  chdir("/");
#endif

#if 1
  fd = open("/dev/tty", O_RDWR);
  write(fd, "hello world\n", sizeof("hello world\n"));
  if (fd < 0) {
    printf("open(/dev/console) failed:%s\n", strerror(errno));
  } else {
    dup2(fd, 0);
    dup2(fd, 1);
    dup2(fd, 2);
    close(fd);
  }
#endif

  // Enter the kernel command processor.
  do_commands(argv[0]);
}

