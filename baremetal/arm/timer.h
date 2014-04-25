#ifndef _timer_h_
#define _timer_h_

/** Get the timer resolution in nanoseconds.
 */
long timer_getres(void);

/** Get the current number of nanoseconds left in the current second.
 */
long timer_getns(void);

/** Set the nanosecond timeout function.
 * This function is called by the interrupt handler when the timer expires.
 */
void timer_set_ns_handler(void (*fn)(void));

/** Set the second timeout function.
 * This function is called by the interrupt handler when the timer expires.
 */
void timer_set_sec_handler(void (*fn)(void));

#endif // _timer_h_
