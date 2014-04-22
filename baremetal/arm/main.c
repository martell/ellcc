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

#define THREAD   // This doesn't work yet.
#if defined(THREAD)
static void *thread(void *arg)
{
    return NULL;
}
#endif

long __syscall_ret(unsigned long r);
long __syscall(long, ...);

#define CONTEXT
#if defined(CONTEXT)

Queue queue = {};
static intptr_t context(intptr_t arg1, intptr_t arg2)
{
    for ( ;; ) {
      Message *msg = get_message(&queue);
      printf("hello from context %" PRIdPTR " code = %d\n", arg1, msg->code);
    }
    return 0;
}
#endif

int main(int argc, char **argv)
{
    printf("%s: hello world\n", argv[0]);

    int i = __syscall_ret(__syscall(0, 1, 2, 3, 4, 5, 6));
    printf("__syscall(0) = %d, %s\n", i, strerror(errno));
    
#if defined(THREAD)
    int s;
    pthread_attr_t attr;
    s = pthread_attr_init(&attr);
    pthread_t id;
    if (s != 0)
        printf("pthread_attr_init: %s\n", strerror(errno));
    void *sp = malloc(4096);
    s = pthread_attr_setstack(&attr, sp, 4096);
    if (s != 0)
        printf("pthread_attr_setstack %s\n", strerror(errno));
    s = pthread_create(&id, &attr, &thread, NULL);
    if (s != 0)
        printf("pthread_create: %s\n", strerror(errno));
#endif

#if defined(CONTEXT)
    new_thread(context, 4096, 42, 0);
    Message msg = { { NULL, sizeof(msg) }, 3 };
    send_message(&queue, &msg);
    sched_yield();      // Let the other thread run.
    msg.code = 6809;
    send_message(&queue, &msg);
    sched_yield();      // Let the other thread run.
#endif

    for ( ;; ) {
        char buffer[100];
        fputs("prompt: ", stdout);
        fflush(stdout);
        fgets(buffer, sizeof(buffer), stdin);
        printf("got: %s", buffer);
    }
}
