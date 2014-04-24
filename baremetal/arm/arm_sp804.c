/* Initialize the ARM SP804 dual timer.
 */

#include "kernel.h"
#include "timer.h"
#include "arm_sp804.h"

static long resolution;         // The clock divisor.
static long accumulated_error;  // Accumulated error in nanoseconds.
static void (*handler)(void);   // The timeout handler.

/* Get the monotonic timer resolution.
 */
long timer_monotonic_getres(void)
{
    return resolution; 
}

/* Get the realtime timer resolution.
 */
long timer_realtime_getres(void)
{
    return resolution; 
}

/** Set the timeout function.
 * This function is called by the interrupt handler when the timer expires.
 */
void timer_set_handler(void (*fn)(void))
{
    handler = fn;
}

/** Set the next timeout.
 * @param value The timeout period in nanoseconds.
 */
void timer_set_timeout(long value)
{
    long remainder = value % resolution;    // How many nanoseconds are we losing?
    value /= resolution;
    accumulated_error += remainder;         // Accumulate the error.
    if (accumulated_error >= resolution) {
        // Have accumulated enough error to bump the time.
        value += 1;
        accumulated_error -= resolution;
    }
}

static void init(void)
    __attribute__((__constructor__, __used__));

static void init(void)
{
    // Set up the timer.
    resolution = 1000000000 / (CLOCK / 1); 
}
