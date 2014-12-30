/* Simple socket tests.
 */
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

int main(int argc, char **argv)
{
  int sv[2];
  int s = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
  if (s < 0) {
    printf("socketpair() failed: %s\n", strerror(errno));
    exit(1);
  }

  s = write(sv[0], "hello world\n", sizeof( "hello world\n"));
  if (s < 0) {
    printf("write() failed: %s\n", strerror(errno));
  }
  char buffer[100];
  s = read(sv[1], buffer, 1);
  if (s < 0) {
    printf("read() failed: %s\n", strerror(errno));
  }

  s = mknod("/socket", S_IFSOCK|S_IRWXU, 0);
  if (s < 0) {
    printf("mknod() failed: %s\n", strerror(errno));
  }

  int fd = open("/socket", O_RDWR);
  if (fd < 0) {
    printf("open() failed: %s\n", strerror(errno));
  }
  s = read(fd, buffer, 1);
  if (s < 0) {
    printf("read() failed: %s\n", strerror(errno));
  }
}

