/** Some test cases.
 * This file adds command to test functionality of the kernel.
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#include "command.h"
#include "kernel.h"

long __syscall_ret(unsigned long r);
long __syscall(long, ...);

static int syscallCommand(int argc, char **argv)
{
    if (argc <= 0) {
        printf("test the syscall interface with an unhandled system call.\n");
        return COMMAND_OK;
    }

    int i = __syscall_ret(__syscall(0, 1, 2, 3, 4, 5, 6));
    printf("__syscall(0) = %d, %s\n", i, strerror(errno));
    return COMMAND_OK;
}

static int yieldCommand(int argc, char **argv)
{
    if (argc <= 0) {
        printf("yield the current time slice.\n");
        return COMMAND_OK;
    }

    sched_yield();
    return COMMAND_OK;
}

/** A simple thread.
 */
static Thread *my_thread;
static void *thread1(void *arg)
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

static int thread1Command(int argc, char **argv)
{
    if (argc <= 0) {
        printf("start the thread1 test case.\n");
        return COMMAND_OK;
    }

    pthread_attr_t attr;
    int s = pthread_attr_init(&attr);
    if (s != 0)
        printf("pthread_attr_init: %s\n", strerror(errno));
    void *sp = malloc(4096);
    s = pthread_attr_setstack(&attr, sp, 4096);
    if (s != 0)
        printf("pthread_attr_setstack %s\n", strerror(errno));
    pthread_t id;
    s = pthread_create(&id, &attr, &thread1, "foo");
    if (s != 0)
        printf("pthread_create: %s\n", strerror(errno));
    sched_yield();

    return COMMAND_OK;
}

static int send1Command(int argc, char **argv)
{
    if (argc <= 0) {
        printf("send a message to the thread1 test thread.\n");
        return COMMAND_OK;
    }

    static Message msg = { 3 };
    send_message(&my_thread->queue, msg);
    msg.code++;
    sched_yield();
    return COMMAND_OK;
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

static int thread2Command(int argc, char **argv)
{
    if (argc <= 0) {
        printf("start the thread2 test case.\n");
        return COMMAND_OK;
    }

    pthread_attr_t attr;
    int s = pthread_attr_init(&attr);
    if (s != 0)
        printf("pthread_attr_init: %s\n", strerror(errno));
    char *sp = malloc(4096);
    s = pthread_attr_setstack(&attr, sp, 4096);
    if (s != 0)
        printf("pthread_attr_setstack %s\n", strerror(errno));
    pthread_t id2;
    s = pthread_create(&id2, &attr, &thread2, NULL);
    if (s != 0)
        printf("pthread_create: %s\n", strerror(errno));
    sched_yield();
    return COMMAND_OK;
}

int sectionCommand(int argc, char **argv)
{
    if (argc <= 0 ) {
        printf("Test Commands:\n");
    }
    return COMMAND_OK;
}

/* Initialize the test cases.
 */
static void init(void)
    __attribute__((__constructor__, __used__));

static void init(void)
{
    command_insert(NULL, sectionCommand);
    command_insert("syscall", syscallCommand);
    command_insert("yield", yieldCommand);
    command_insert("thread1", thread1Command);
    command_insert("send1", send1Command);
    command_insert("thread2", thread2Command);
}
