/* A simple bare metal main.
 */

#undef THREAD

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

#if defined(THREAD)
static void *thread(void *arg)
{
    return NULL;
}
#endif

int main(int argc, char **argv)
{
    // __syscall(1, 2, 3, 4, 5, 6, 7);
    printf("%s: hello world\n", argv[0]);
    printf("hello world\n");
    
#if defined(THREAD)
    int s;
    pthread_attr_t attr;
    s = pthread_attr_init(&attr);
    pthread_t id;
    if (s != 0)
        printf("pthread_attr_init: %s\n", strerror(errno));
    s = pthread_create(&id, &attr, &thread, NULL);
    if (s != 0)
        printf("pthread_create: %s\n", strerror(errno));
#endif
    for ( ;; ) {
        char buffer[100];
        fgets(buffer, sizeof(buffer), stdin);
        printf("got: %s", buffer);
    }
}


