/** Some test cases.
 * This file adds command to test functionality of the kernel.
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <sched.h>
#include <unistd.h>
#include "command.h"
#include "scheduler.h"

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
static void *id1;
static intptr_t thread1(intptr_t arg1, intptr_t arg2)
{
    printf ("thread started %s\n", (char *)arg1);
    for ( ;; ) {
        // Go to sleep.
        Message msg = get_message(NULL);
        printf ("thread running %d\n", msg.code);
    }

    return 0;
}

static int thread1Command(int argc, char **argv)
{
    if (argc <= 0) {
        printf("start the thread1 test case.\n");
        return COMMAND_OK;
    }

    new_thread("thread1",       // name
               &id1,            // id
               thread1,         // entry
               0,               // priority
               NULL,            // stack
               4096,            // stack size
               (intptr_t)"foo", // arg1
               0);              // arg2

    return COMMAND_OK;
}

static int test1Command(int argc, char **argv)
{
    if (argc <= 0) {
        printf("send a message to the thread1 test thread.\n");
        return COMMAND_OK;
    }

    if (!id1) {
        printf("thread1 has not been started.\n");
        return COMMAND_ERROR;
    }

    static Message msg = { 3 };
    send_message(&((Thread *)id1)->queue, msg);
    msg.code++;
    return COMMAND_OK;
}

static int cancel1Command(int argc, char **argv)
{
    if (argc <= 0) {
        printf("cancel the thread1 test thread.\n");
        return COMMAND_OK;
    }

    if (pthread_cancel(id1) == 0) {
        return COMMAND_OK;
    } else {
        return COMMAND_ERROR;
    }
}

static intptr_t thread2(intptr_t arg1, intptr_t arg2)
{
    printf ("thread2 started\n");
    for ( ;; ) {
        // Go to sleep.
        sleep(10);
        printf ("thread2 still running\n");
    }

    return 0;
}

static int thread2Command(int argc, char **argv)
{
    if (argc <= 0) {
        printf("start the thread2 test case.\n");
        return COMMAND_OK;
    }

    void *id2;
    new_thread("thread2",       // name
               &id2,            // id
               thread2,         // entry
               0,               // priority
               NULL,            // stack
               4096,            // stack size
               0,               // arg1
               0);              // arg2
    return COMMAND_OK;
}

/** A pthread thread.
 */
static int counter;
static void *thread3(void *arg)
{
    printf ("thread3 started\n");
    for ( ;; ) {
        // Very busy.
        ++counter;
    }

    return NULL;
}

static int thread3Command(int argc, char **argv)
{
    if (argc <= 0) {
        printf("start the thread3 test case.\n");
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
    pthread_t id3;
    s = pthread_create(&id3, &attr, &thread3, NULL);
    if (s != 0)
        printf("pthread_create: %s\n", strerror(errno));
    return COMMAND_OK;
}

static int test3Command(int argc, char **argv)
{
    if (argc <= 0) {
        printf("test the thread3 test case.\n");
        return COMMAND_OK;
    }

    printf("counter = %d\n", counter);
    return COMMAND_OK;
}

int sectionCommand(int argc, char **argv)
{
    if (argc <= 0 ) {
        printf("Test Commands:\n");
    }
    return COMMAND_OK;
}

/** A pthread thread.
 */
static sem_t sem4;
static pthread_t id4;
static void *thread4(void *arg)
{
    printf ("thread4 started\n");
    for ( ;; ) {
        sem_wait(&sem4);
        printf("thread4 running\n");
    }

    return NULL;
}

static int thread4Command(int argc, char **argv)
{
    if (argc <= 0) {
        printf("start the thread4 test case.\n");
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
    s = pthread_create(&id4, &attr, &thread4, NULL);
    if (s != 0)
        printf("pthread_create: %s\n", strerror(errno));

    s = sem_init(&sem4, 0, 0);
    if (s != 0)
        printf("sem_init: %s\n", strerror(errno));
    return COMMAND_OK;
}

static int test4Command(int argc, char **argv)
{
    if (argc <= 0) {
        printf("test the thread3 test case.\n");
        return COMMAND_OK;
    }

    if (!id4) {
        printf("thread4 has not been started.\n");
        return COMMAND_ERROR;
    }

    printf("counter = %d\n", counter);
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
    command_insert("test1", test1Command);
    command_insert("cancel1", cancel1Command);
    command_insert("thread2", thread2Command);
    command_insert("thread3", thread3Command);
    command_insert("test3", test3Command);
    command_insert("thread4", thread4Command);
    command_insert("test4", test4Command);
}
