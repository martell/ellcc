/*******************************************/
/* Simple Bare metal program init */
/*******************************************/

#include <bits/syscall.h>
#include <sys/uio.h>
#include <stdio.h>
#include "kernel.h"

/* Note: QEMU model of PL011 serial port ignores the transmit
 * FIFO capabilities. When writing on a real SOC, the
 * "Transmit FIFO Full" flag must be checked in UARTFR register
 * before writing on the UART register*/

volatile unsigned int* const UART0 = (unsigned int*)0x0101F1000;

static ssize_t sys_write(int fd, const void *buf, size_t count)
{
    const unsigned char *s = buf;
    size_t i = 0;
    for ( ; i < count; ++i) {
        *UART0 = *s++;
    }
    return i;
}

static ssize_t sys_writev(int fd, const struct iovec *iov, int iovcount)
{
    ssize_t count = 0;
    for (int i = 0; i < iovcount; ++i) {
        ssize_t c = sys_write(fd, iov[i].iov_base, iov[i].iov_len);
        if (c < 0) {
            count = c;
            break;
        }
        count += c;
    }
    return count;
}

/* Main entry point */
void _startup(void)
{
    // Set up a simple write system call.
    __set_syscall(SYS_write, sys_write);
    // Set up a simple writev system call.
    __set_syscall(SYS_writev, sys_writev);
    // Test it.
    printf("hello world\n");
}


