/* Just enough to get toybox to compile.
 */
#ifndef _LINUX_RTC_H_
#define _LINUX_RTC_H_

#define RTC_RD_TIME  _IOR('p', 0x09, struct tm)
#define RTC_SET_TIME _IOW('p', 0x0a, struct tm)

#endif /* _LINUX_RTC_H_ */
