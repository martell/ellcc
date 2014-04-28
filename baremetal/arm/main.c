/* A simple bare metal main.
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#include "kernel.h"
#include "arm.h"

#define THREAD
#if defined(THREAD)
Thread *my_thread;
static void *thread(void *arg)
{
    my_thread = __get_self();
    printf ("thread started %s\n", (char *)arg);
    printf("thread self = 0x%08X\n", (unsigned)__get_self());
    for ( ;; ) {
        // Go to sleep.
        Message msg = get_message(NULL);
        printf ("thread running %d\n", msg.code);
    }

    return NULL;
}
static void *thread2(void *arg)
{
    printf ("thread2 started\n");
    for ( ;; ) {
        // Go to sleep.
        sleep(10);
        printf ("thread2 still running\n");
    }

    return NULL;
}
#endif

long __syscall_ret(unsigned long r);
long __syscall(long, ...);

int main(int argc, char **argv)
{
#if 1
    printf("%s: hello world\n", argv[0]);

    int i = __syscall_ret(__syscall(0, 1, 2, 3, 4, 5, 6));
    printf("__syscall(0) = %d, %s\n", i, strerror(errno));
#endif
    int s;
#if defined(THREAD)
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
    printf("thread id = 0x%08X\n", (unsigned) id);
    sched_yield();      // Let the other thread run.
    s = pthread_attr_init(&attr);
    if (s != 0)
        printf("pthread_attr_init: %s\n", strerror(errno));
    sp = malloc(4096);
    s = pthread_attr_setstack(&attr, sp, 4096);
    if (s != 0)
        printf("pthread_attr_setstack %s\n", strerror(errno));
    pthread_t id2;
    s = pthread_create(&id2, &attr, &thread2, NULL);
    if (s != 0)
        printf("pthread_create: %s\n", strerror(errno));
    Message msg = { 3 };
#endif

    struct timespec real = { 12345, 0 };
    clock_settime(CLOCK_REALTIME, &real);
    printf("main self = 0x%08X\n", (unsigned)pthread_self());
    for ( ;; ) {
        char buffer[100];
        fputs("prompt: ", stdout);
        fflush(stdout);
        fgets(buffer, sizeof(buffer), stdin);
        printf("got: %s", buffer);
#if 1
#if defined(THREAD)
        send_message(&my_thread->queue, msg);
        msg.code++;
        sched_yield();      // Let the other thread run.
#endif
        struct timespec req = { 3, 30000000 };

#if 1
        struct timespec rem;
        s = clock_gettime(CLOCK_REALTIME, &req);
        if (s != 0)
            printf("clock_gettime: %s\n", strerror(errno));
        printf("before nanosleep current realtime: %ld.%09ld\n", req.tv_sec, req.tv_nsec);

        struct timespec nreq = { 0, 30000000 };
        s = nanosleep(&nreq, &rem);
        if (s != 0)
            printf("nanosleep: %s\n", strerror(errno));
        
        s = clock_gettime(CLOCK_REALTIME, &req);
        if (s != 0)
            printf("clock_gettime: %s\n", strerror(errno));
        printf("after nanosleep current realtime: %ld.%09ld\n", req.tv_sec, req.tv_nsec);
#endif

        s = clock_gettime(CLOCK_MONOTONIC, &req);
        printf("current monotonic: %ld.%09ld\n", req.tv_sec, req.tv_nsec);
        if (s != 0)
            printf("clock_gettime: %s\n", strerror(errno));
        s = clock_gettime(CLOCK_REALTIME, &req);
        if (s != 0)
            printf("clock_gettime: %s\n", strerror(errno));
        printf("before sleep current realtime: %ld.%09ld\n", req.tv_sec, req.tv_nsec);
        sleep(3);
        s = clock_gettime(CLOCK_REALTIME, &req);
        if (s != 0)
            printf("clock_gettime: %s\n", strerror(errno));
        printf("after sleep current realtime: %ld.%09ld\n", req.tv_sec, req.tv_nsec);
#endif
    }
}
