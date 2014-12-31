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
  int s = socketpair(AF_UNIX, SOCK_STREAM|SOCK_NONBLOCK, 0, sv);
  if (s < 0) {
    printf("socketpair() failed: %s\n", strerror(errno));
    exit(1);
  }

  s = write(sv[0], "hello world\n", sizeof("hello world\n"));
  if (s < 0) {
    printf("write() failed: %s\n", strerror(errno));
  }
  char buffer[100];
  s = read(sv[1], buffer, 100);
  if (s < 0) {
    printf("read() failed: %s\n", strerror(errno));
  } else {
    printf("read returned s = %d, '%s'\n", s, buffer);
  }
  s = write(sv[0], "foo world\n", sizeof("foo world\n"));
  if (s < 0) {
    printf("write() failed: %s\n", strerror(errno));
  }
  s = read(sv[1], buffer, 100);
  if (s < 0) {
    printf("read() failed: %s\n", strerror(errno));
  } else {
    printf("read returned s = %d, '%s'\n", s, buffer);
  }
}

