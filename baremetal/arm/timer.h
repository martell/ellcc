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

/** Make an entry in the sleeping list and sleep
 * or schedule a callback.
 */
typedef void (*TimerCallback)(intptr_t);
void *timer_wake_at(long long when, TimerCallback callback, intptr_t arg);

/** Cancel a previously scheduled wakeup.
 * This function will cancel a previously scheduled wakeup.
 * If the wakeup caused the caller to sleep, it will be rescheduled.
 * @param id The timer id.
 * @return 0 if cancelled, else the timer has probably already expired.
 */
int timer_cancel_wake_at(void *id);

#endif // _timer_h_
