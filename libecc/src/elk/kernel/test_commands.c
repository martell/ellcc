/** Some test cases.
 * This file adds commands to test functionality of the kernel.
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#include <semaphore.h>
#include <command.h>
#include <thread.h>

// Make the test commands a loadable feature.
FEATURE(test_commands, test_commands)

static int yieldCommand(int argc, char **argv)
{
  if (argc <= 0) {
    printf("yield the current time slice.\n");
    return COMMAND_OK;
  }

  sched_yield();
  return COMMAND_OK;
}

static int thread_create(const char *name, pthread_t *id,
                         void *(*start)(void *), int pri,
                         void *stack, size_t stack_size,
                         void *arg)
{
  pthread_attr_t attr;
  int s = pthread_attr_init(&attr);
  if (s != 0)
    printf("pthread_attr_init: %s\n", strerror(s));
  if (stack == NULL) {
    stack = malloc(stack_size);
  }
  s = pthread_attr_setstack(&attr, stack, stack_size);
  if (s != 0)
    printf("pthread_attr_setstack %s\n", strerror(s));
  s = pthread_create(id, &attr, start, arg);
  if (s != 0)
    printf("pthread_create: %s\n", strerror(s));
  return s;
}

/** A simple thread.
 */
static int tid1;
static pthread_t id1;
static void *thread1(void *arg)
{
  tid1 = gettid();
  printf ("thread started %s\n", (char *)arg);
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

  thread_create("thread1",              // name
                &id1,                   // id
                thread1,                // entry
                0,                      // priority
                NULL,                   // stack
                4096,                   // stack size
                "foo");                 // arg

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
  int s = __elk_send_message(tid1, msg);
  if (s != 0) {
    printf("__elk_send_message to %d: %s\n", tid1, strerror(s));
  }
  msg.code++;
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

  return 0;
}

static int thread2Command(int argc, char **argv)
{
  if (argc <= 0) {
    printf("start the thread2 test case.\n");
    return COMMAND_OK;
  }

  pthread_t id2;
  thread_create("thread2",              // name
                &id2,                   // id
                thread2,                // entry
                0,                      // priority
                NULL,                   // stack
                4096,                   // stack size
                NULL);                  // arg

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
    sleep(10);
    return (void*)0xdeadbeef;
  }

  return NULL;
}

static pthread_t id3;
static int thread3Command(int argc, char **argv)
{
  if (argc <= 0) {
    printf("start the thread3 test case.\n");
    return COMMAND_OK;
  }

  pthread_attr_t attr;
  int s = pthread_attr_init(&attr);
  if (s != 0)
    printf("pthread_attr_init: %s\n", strerror(s));
  char *sp = malloc(4096);
  s = pthread_attr_setstack(&attr, sp, 4096);
  if (s != 0)
    printf("pthread_attr_setstack %s\n", strerror(s));
  s = pthread_create(&id3, &attr, &thread3, NULL);
  if (s != 0)
    printf("pthread_create: %s\n", strerror(s));
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

static int cancel3Command(int argc, char **argv)
{
  if (argc <= 0) {
    printf("cancel the thread3 test thread.\n");
    return COMMAND_OK;
  }

  if (pthread_cancel(id3) == 0) {
    return COMMAND_OK;
  } else {
    return COMMAND_ERROR;
  }
}

static int join3Command(int argc, char **argv)
{
  if (argc <= 0) {
    printf("join the thread3 test thread.\n");
    return COMMAND_OK;
  }

  void *retval;
  int s = pthread_join(id3, &retval);
  if (s == 0) {
    printf("joined with thread3: %p\n", retval);
    return COMMAND_OK;
  } else {
    printf("pthread_join() failed: %s\n", strerror(s));
    return COMMAND_ERROR;
  }
}

static sem_t sem4;
static pthread_t id4;
static void *thread4(void *arg)
{
  printf ("thread4 started\n");
  for ( ;; ) {
    sem_wait(&sem4);
    printf("thread4 running\n");
  }

  return 0;
}

static int thread4Command(int argc, char **argv)
{
  if (argc <= 0) {
    printf("start the thread4 test case.\n");
    return COMMAND_OK;
  }

  int s = sem_init(&sem4, 0, 0);
  if (s != 0)
    printf("sem_init: %s\n", strerror(errno));

  thread_create("thread4",              // name
                &id4,                   // id
                thread4,                // entry
                0,                      // priority
                NULL,                   // stack
                4096,                   // stack size
                NULL);                  // arg

  return COMMAND_OK;
}

static int test4Command(int argc, char **argv)
{
  if (argc <= 0) {
    printf("test the thread4 test case.\n");
    return COMMAND_OK;
  }

  if (!id4) {
    printf("thread4 has not been started.\n");
    return COMMAND_ERROR;
  }

  sem_post(&sem4);
  return COMMAND_OK;
}

static pthread_t id5;
static int me5;
static void *thread5(void *arg)
{
  int me = (int)arg;
  printf ("thread5 started\n");
  sem_t sem5;
  int s = sem_init(&sem5, 0, 0);
  if (s != 0) {
    printf("sem_init: %s\n", strerror(errno));
    return NULL;
  }

  for ( ;; ) {
    struct timespec ts = { 10, 0 };
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    ts.tv_sec += now.tv_sec;
    ts.tv_nsec += now.tv_nsec;
    if (ts.tv_nsec >= 1000000000) {
      ts.tv_sec += 1;
      ts.tv_nsec -= 1000000000;
    }
    sem_timedwait(&sem5, &ts);
    printf("thread5 running %d\n", me);
  }

  return 0;
}

static int thread5Command(int argc, char **argv)
{
  if (argc <= 0) {
    printf("start the thread5 test case.\n");
    return COMMAND_OK;
  }

  ++me5;
  thread_create("thread5",              // name
                &id5,                   // id
                thread5,                // entry
                0,                      // priority
                NULL,                   // stack
                4096,                   // stack size
                (void *)(intptr_t)me5); // arg

  return COMMAND_OK;
}

long __syscall_ret(unsigned long r);
long __syscall(long, ...);

static int syscallCommand(int argc, char **argv)
{
  if (argc <= 0) {
    printf("test the syscall interface with an unhandled system call.\n");
    return COMMAND_OK;
  }

  int i = __syscall_ret(__syscall(0, 10, 20, 30, 40, 50, 60));
  printf("__syscall(0) = %d, %s\n", i, strerror(errno));
  return COMMAND_OK;
}

/** Create a section heading for the help command.
 */
static int sectionCommand(int argc, char **argv)
{
  if (argc <= 0 ) {
    printf("Test Commands:\n");
  }
  return COMMAND_OK;
}

/* Initialize the test commands.
 */
ELK_CONSTRUCTOR()
{
  command_insert(NULL, sectionCommand);
  command_insert("syscall", syscallCommand);
  command_insert("yield", yieldCommand);
  command_insert("thread1", thread1Command);
  command_insert("test1", test1Command);
  command_insert("cancel3", cancel3Command);
  command_insert("join3", join3Command);
  command_insert("thread2", thread2Command);
  command_insert("thread3", thread3Command);
  command_insert("test3", test3Command);
  command_insert("thread4", thread4Command);
  command_insert("test4", test4Command);
  command_insert("thread5", thread5Command);
}
