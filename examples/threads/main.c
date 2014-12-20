#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

/** A pthread thread.
 */
static void *thread(void *arg)
{
  printf ("thread started\n");
  // sleep(2);
  return (void*)0xdeadbeef;
}

int main()
{
  pthread_t id;
  int s;

  int i = 0;
  while (1) {
    s = pthread_create(&id, NULL, &thread, NULL);
    if (s != 0) {
      printf("pthread_create: %s\n", strerror(s));
      exit(1);
    }

    ++i;

    void *retval;
    s = pthread_join(id, &retval);
    if (s == 0) {
      printf("[%d]: joined with thread: %p\n", i, retval);
    } else {
      printf("pthread_join() failed: %s\n", strerror(s));
    }
  }
}
