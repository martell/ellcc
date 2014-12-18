#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

/** A pthread thread.
 */
static void *thread(void *arg)
{
  printf ("thread started\n");
  sleep(1);
  return (void*)0xdeadbeef;
}

int main()
{
  pthread_t id;
  int s;

  while (1) {
    s = pthread_create(&id, NULL, &thread, NULL);
    if (s != 0) {
      printf("pthread_create: %s\n", strerror(s));
    }

    void *retval;
    s = pthread_join(id, &retval);
    if (s == 0) {
      printf("joined with thread3: %p\n", retval);
    } else {
      printf("pthread_join() failed: %s\n", strerror(s));
    }
  }
}
