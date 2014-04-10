/* A simple bare metal main.
 * We define enough functionality here to get printf and friends
 * working with a simple polled serial interface for debugging.
 */

#include <bits/syscall.h>       // For syscall numbers.
#include <sys/uio.h>            // For writev (used by printf().
#include <stdio.h>
#include "kernel.h"

/* Define to core (and soon to be superceeded) system call
 * implimentaions: write() and writev().
 */
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

int main(int argc, char **argv)
{
    // Set up a simple write system call.
    __set_syscall(SYS_write, sys_write);
    // Set up a simple writev system call.
    __set_syscall(SYS_writev, sys_writev);
    // __syscall(1, 2, 3, 4, 5, 6, 7);
    // Test it.
    printf("%s: hello world\n", argv[0]);
    printf("hello world\n");
    // RICH: Loop forever here until ARM cas is fixed.
    for ( ;; )
        continue;
}


