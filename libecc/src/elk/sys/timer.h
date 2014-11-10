/** System timer handling.
 */
#ifndef _timer_h_
#define _timer_h_

/** Get the timer resolution in nanoseconds.
 * This function is supplied by platform specific code. e.g. arm/arm_sp804.c
 */
long __elk_timer_getres(void);

/** Set the realtime timer.
 * This function is supplied by platform specific code. e.g. arm/arm_sp804.c
 */
void __elk_timer_set_realtime(long long value);

/** Get the realtime timer.
 * This function is supplied by platform specific code. e.g. arm/arm_sp804.c
 */
long long __elk_timer_get_realtime(void);

/** Get the monotonic timer.
 * This function is supplied by platform specific code. e.g. arm/arm_sp804.c
 */
long long __elk_timer_get_monotonic(void);

/* Get the realtime offset.
 * This function is supplied by platform specific code. e.g. arm/arm_sp804.c
 */
long long __elk_timer_get_realtime_offset(void);

/** Start the sleep timer.
 * This function is supplied by platform specific code. e.g. arm/arm_sp804.c
 */
void __elk_timer_start(long long when);

#endif // _timer_h_
