#ifndef _timer_h_
#define _timer_h_

/** Get the timer resolution in nanoseconds.
 */
long timer_getres(void);

/** Get the realtime timer.
 */
long long timer_get_realtime(void);

/** Set the realtime timer.
 */
void timer_set_realtime(long long value);

/** Get the monotonic timer.
 */
long long timer_get_monotonic(void);

/** Set the timeout function.
 * This function is called by the interrupt handler when the timer expires.
 */
void timer_set_handler(void (*fn)(long long monotonic));

#endif // _timer_h_
