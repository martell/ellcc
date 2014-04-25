/* A simple bare metal main.
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sched.h>
#include "kernel.h"
#include "arm.h"

#define THREAD
#if defined(THREAD)
Queue thread_queue = {};
static void *thread(void *arg)
{
    printf ("thread started %s\n", (char *)arg);
    for ( ;; ) {
        // Go to sleep.
        get_message(&thread_queue);
        printf ("thread running\n");
    }

    return NULL;
}
#endif

long __syscall_ret(unsigned long r);
long __syscall(long, ...);

int main(int argc, char **argv)
{
    printf("%s: hello world\n", argv[0]);

    int i = __syscall_ret(__syscall(0, 1, 2, 3, 4, 5, 6));
    printf("__syscall(0) = %d, %s\n", i, strerror(errno));
    
#if defined(THREAD)
    int s;
    pthread_attr_t attr;
    s = pthread_attr_init(&attr);
    if (s != 0)
        printf("pthread_attr_init: %s\n", strerror(errno));
    void *sp = malloc(4096);
    s = pthread_attr_setstack(&attr, sp, 4096);
    if (s != 0)
        printf("pthread_attr_setstack %s\n", strerror(errno));
    pthread_t id;
    s = pthread_create(&id, &attr, &thread, "foo");
    if (s != 0)
        printf("pthread_create: %s\n", strerror(errno));
    sched_yield();      // Let the other thread run.
#endif

    struct timespec real = { 12345, 0 };
    for ( ;; ) {
        char buffer[100];
        fputs("prompt: ", stdout);
        fflush(stdout);
        fgets(buffer, sizeof(buffer), stdin);
        printf("got: %s", buffer);
#if defined(THREAD)
        Message msg = { { NULL, sizeof(msg) }, 3 };
        send_message(&thread_queue, &msg);
        sched_yield();      // Let the other thread run.
#endif
        struct timespec req = { 1, 0 };
        struct timespec rem;

        s = nanosleep(&req, &rem);
        if (s != 0)
            printf("nanosleep: %s\n", strerror(errno));
        
        clock_settime(CLOCK_REALTIME, &real);
        s = clock_gettime(CLOCK_MONOTONIC, &req);
        printf("current monotonic: %ld.%09ld\n", req.tv_sec, req.tv_nsec);
        if (s != 0)
            printf("clock_gettime: %s\n", strerror(errno));
        s = clock_gettime(CLOCK_REALTIME, &req);
        if (s != 0)
            printf("clock_gettime: %s\n", strerror(errno));
        printf("current realtime: %ld.%09ld\n", req.tv_sec, req.tv_nsec);
        real.tv_sec++;
        real.tv_nsec = 0;
    }
}
