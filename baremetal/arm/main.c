/* A simple bare metal main.
 */

#undef THREAD

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include "kernel.h"
#include "arm.h"

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
static void *main_sa;
static void *context_sa;

static int context(intptr_t arg1, intptr_t arg2)
{
    for ( ;; ) {
      printf("hello from context %" PRIdPTR ", %" PRIdPTR "\n", arg1, arg2);
      __switch(&context_sa, main_sa);
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
    s = pthread_create(&id, &attr, &thread, NULL);
    if (s != 0)
        printf("pthread_create: %s\n", strerror(errno));
#endif
#if defined(CONTEXT)
    char *p = malloc(4096);
    context_sa = p + 4096;
    __new_context(&context_sa, context, Mode_SYS, NULL, 42, 6809);
    __switch(&main_sa, context_sa);
    __switch(&main_sa, context_sa);

#endif
    for ( ;; ) {
        char buffer[100];
        fgets(buffer, sizeof(buffer), stdin);
        printf("got: %s", buffer);
    }
}


