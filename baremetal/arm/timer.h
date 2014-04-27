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

/** Start the sleep timer.
 */
void timer_start(long long when);

/** Timer expired handler.
 * This function is called in an interrupt context.
 */
long long timer_expired(long long when);

#endif // _timer_h_
