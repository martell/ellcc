/* Initialize the ARM SP804 dual timer for use as the system clock.
 * RICH: This should be renamed to something a little less generic.
 */

#include <pthread.h>
#include <time.h>
#include "kernel.h"
#include "timer.h"
#include "thread.h"
#include "irq.h"
#include "arm_sp804.h"

static long resolution;                     // The clock divisor.
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static volatile time_t monotonic_seconds;   // The monotonic timer.
static long long realtime_offset;           // The realtime timer offset.
static int timeout_active;                  // Set if a timeout is active.
static long long timeout;                   // When the next timeout occurs.

/* Get the timer resolution.
 */
long timer_getres(void)
{
  return resolution;
}

/* Get the realtime offset.
 */
long long timer_get_realtime_offset(void)
{
  return realtime_offset;
}

/** Get the monotonic timer.
 */
long long timer_get_monotonic(void)
{
  time_t secs;
  long nsecs;
  do {
    secs = monotonic_seconds;
    nsecs = 1000000000 - (*Timer2Value * resolution);
  } while (secs != monotonic_seconds);    // Take care of a seconds update.

  long long value = secs * 1000000000LL + nsecs;
  return value;
}

/** Get the realtime timer.
 */
long long timer_get_realtime(void)
{
  long long value = timer_get_monotonic();
  pthread_mutex_lock(&mutex);
  value += realtime_offset;
  pthread_mutex_unlock(&mutex);
  return value;
}

/** Set the realtime timer.
 */
void timer_set_realtime(long long value)
{
  long long mt = timer_get_monotonic();
  pthread_mutex_lock(&mutex);
  realtime_offset = value - mt;
  pthread_mutex_unlock(&mutex);
}

/** Check to see if a timeout interrupt is needed.
 */
static void check_timeout()
{
  if (!timeout_active) {
    return;
  }

  long long mt = timer_get_monotonic();
  if (timeout - mt > 1000000000) {
    // More than a second away.
    return;
  }

  // In the same second. Set up the interrupt.
  timeout_active = 0;

  // Set up Timer 1 as the short term timer.
  mt = timeout - mt;
  mt = mt * SP804_CLOCK / 1000000000;
  if (mt < 0) mt = 0;
  *Timer1Load = mt;
  // Enable timer, 32 bit, Divide by 1 clock, oneshot.
  *Timer1Control = TimerEn|TimerSize|IntEnable|OneShot;
}

/** This is the second timer interupt handler.
 */
static void sec_interrupt(void *arg)
{
  monotonic_seconds++;
  check_timeout();
}

/** Start the sleep timer.
 */
void timer_start(long long when)
{
  long long mt;
  do {
    mt = timer_get_monotonic();
    if (when <= mt) {
      when = timer_expired(mt);
      if (when == 0) {
        timeout_active = 0;
        return;
      }
    }
  } while (when <= mt);
  pthread_mutex_lock(&mutex);
  timeout_active = 1;
  timeout = when;
  check_timeout();
  pthread_mutex_unlock(&mutex);
}

static void short_interrupt(void *arg)
{
  long long mt = timer_get_monotonic();
  mt = timer_expired(mt);
  if (mt == 0) {
    timeout_active = 0;
    return;
  }
  timer_start(mt);
}

static const IRQHandler timer_irq =
{
  .id = SP804_IRQ + 32,
  .edge = 0,
  .priority = 0,
  .cpus = 0xFFFFFFFF,         // Send to all CPUs.
  .sources = 2,
  {
    { Timer1MIS, TimerInt, Timer1IntClr, 0,
      { short_interrupt, NULL }},
    { Timer2MIS, TimerInt, Timer2IntClr, 0,
      { sec_interrupt, NULL }},
  }
};

C_CONSTRUCTOR()
{
  // Set up the timer.
  resolution = 1000000000 / (SP804_CLOCK / 1); 

  // Set up Timer 2 as the second timer.
  *Timer2BGLoad = SP804_CLOCK;
  // Register the interrupt handler.
  irq_register(&timer_irq);
  // Enable timer, 32 bit, Divide by 1 clock, periodic.
  *Timer2Control = TimerEn|TimerSize|TimerMode|IntEnable;
}
