/** System timer handling.
 */
#ifndef _timer_h_
#define _timer_h_

#include "config.h"

#if ELK_NAMESPACE
#define timer_getres __elk_timer_getres
#define timer_set_realtime __elk_timer_set_realtime
#define timer_get_realtime __elk_timer_get_realtime
#define timer_get_monotonic __elk_timer_get_monotonic
#define timer_get_realtime_offset __elk_timer_get_realtime_offset
#define timer_start __elk_timer_start
#endif

/** Get the timer resolution in nanoseconds.
 * This function is supplied by platform specific code. e.g. arm/arm_sp804.c
 */
long timer_getres(void);

/** Set the realtime timer.
 * This function is supplied by platform specific code. e.g. arm/arm_sp804.c
 */
void timer_set_realtime(long long value);

/** Get the realtime timer.
 * This function is supplied by platform specific code. e.g. arm/arm_sp804.c
 */
long long timer_get_realtime(void);

/** Get the monotonic timer.
 * This function is supplied by platform specific code. e.g. arm/arm_sp804.c
 */
long long timer_get_monotonic(void);

/* Get the realtime offset.
 * This function is supplied by platform specific code. e.g. arm/arm_sp804.c
 */
long long timer_get_realtime_offset(void);

/** Start the sleep timer.
 * This function is supplied by platform specific code. e.g. arm/arm_sp804.c
 */
void timer_start(long long when);

#endif // _timer_h_
