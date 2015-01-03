/* Simple socket tests.
 */
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

static int thread_create(const char *name, pthread_t *id,
                         void *(*start)(void *), int pri,
                         void *stack, size_t stack_size,
                         void *arg)
{
  int s = pthread_create(id, NULL /* &attr */, start, arg);
  if (s != 0)
    printf("pthread_create: %s\n", strerror(s));
  return s;
}

void *start(void * arg)
{
  int sfd = socket(AF_INET, SOCK_STREAM /*|SOCK_NONBLOCK */, 0);
  if (sfd < 0) {
    printf("socket(AF_INET) failed: %s\n", strerror(errno));
  }
  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = inet_addr("127.0.0.1");
  server.sin_port = htons(8888);
  int s = connect(sfd, (struct sockaddr *)&server, sizeof(server));
  if (s < 0) {
    printf("connect(127.0.0.1) failed: %s\n", strerror(errno));
    exit(1);
  }
  printf("connected\n");
  char buffer[100];
  int i = 0;
  while (1) {
    snprintf(buffer, 100, "hello %d", ++i);
    s = send(sfd, buffer, strlen(buffer) + 1, 0);
    if (s < 0) {
      printf("send(127.0.0.1) failed: %s\n", strerror(errno));
      exit(1);
    }
    sleep(1);
  }

  return 0;
}

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

  s = socket(AF_UNIX, SOCK_STREAM|SOCK_NONBLOCK, 0);
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

  int sfd = socket(AF_INET, SOCK_STREAM /*|SOCK_NONBLOCK */, 0);
  if (sfd < 0) {
    printf("socket(AF_INET) failed: %s\n", strerror(errno));
  }
  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(8888);
  s = bind(sfd, (struct sockaddr *)&server, sizeof(server));
  if (s < 0) {
    printf("bind(AF_INET) failed: %s\n", strerror(errno));
  }
  s = listen(sfd, 3);
  if (s < 0) {
    printf("listen(AF_INET) failed: %s\n", strerror(errno));
  }

  pthread_t id;
  thread_create("foo", &id, start, 0, 0, 0, NULL);

  struct sockaddr_in client;
  int clientlen = sizeof(client);
  int cfd = accept(sfd, (struct sockaddr *)&client, (socklen_t *)&clientlen);
  if (cfd < 0) {
    printf("accept(AF_INET) failed: %s\n", strerror(errno));
  }

  printf("connection accepted\n");
  size_t size;
  while((size = recv(cfd, buffer, 100, 0)) > 0) {
    printf("got '%s'\n", buffer);
  }
  printf("exiting\n");
}

