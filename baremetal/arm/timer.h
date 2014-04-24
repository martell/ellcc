#ifndef _timer_h_
#define _timer_h_

/** Get the timer resolution in nanoseconds.
 */
long timer_monotonic_getres(void);
long timer_realtime_getres(void);

#endif // _timer_h_
