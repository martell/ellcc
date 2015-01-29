/* A very simple non-interrupt driven console.
 */
#include <unistd.h>
#include <syscalls.h>           // For syscall numbers.
#include <sys/uio.h>            // For writev (used by printf()).
#include <sys/ioctl.h>
#include <kernel.h>
#include "crt1.h"

// Make the simple console a loadable feature.
FEATURE_CLASS(simple_console, console)

#define SIMPLE_CONSOLE          // No interrupt support needed.
#include "console.h"

static int sys_ioctl(int fd, int request, ...)
{
    switch (request) {
    case TCGETS:
        return 0;       // Yes sir, we are a serial port.
    default:
        return -EINVAL;
    }
}

static ssize_t sys_write(int fd, const void *buf, size_t count)
{
    const unsigned char *s = buf;
    size_t i = 0;
    for ( ; i < count; ++i) {
        console_send_char(*s++);
    }
    return i;
}

static ssize_t sys_writev(int fd, const struct iovec *iov, int iovcount)
{
    ssize_t count = 0;
    for (int i = 0; i < iovcount; ++i) {
        ssize_t c = write(fd, iov[i].iov_base, iov[i].iov_len);
        if (c < 0) {
            count = c;
            break;
        }
        count += c;
    }
    return count;
}

static ssize_t sys_read(int fd, void *buf, size_t count)
{
    unsigned char *s = buf;
    size_t i = 0;
    for ( ; i < count; ) {
        // Get the next character.
        int ch = console_get_char();
        ++i;                    // Count the character.
        switch (ch) {
            // Some simple line handling.
        case 0x7F:
        case '\b':
            // Simple backspace handling.
            --i;
            if (i) {
              console_send_char('\b');
              console_send_char(' ');
              console_send_char('\b');
              --s;
              --i;
            }
            break;
        case '\n':
        case '\r':
            // Make sure we send both a CR and LF.
            console_send_char(ch == '\r' ? '\n' : '\r');
            *s = '\n';          // and send a newline back.
            return i;           // This read is done.
        default:
            // Echo input.
            console_send_char(ch);
            *s++ = ch;          // and send it back.
            break;
        }
    }
    return i;
}

static ssize_t sys_readv(int fd, const struct iovec *iov, int iovcount)
{
    ssize_t count = 0;
    for (int i = 0; i < iovcount; ++i) {
        ssize_t c = read(fd, iov[i].iov_base, iov[i].iov_len);
        if (c < 0) {
            count = c;
            break;
        }
        count += c;
    }
    return count;
}

ELK_PRECONSTRUCTOR()
{
    static int setup;
    if (setup) {
      return;
    }

    // Set up a simple ioctl system call.
    SYSCALL(ioctl);
    // Set up a simple write system call.
    SYSCALL(write);
    // Set up a simple writev system call.
    SYSCALL(writev);
    // Set up a simple read system call.
    SYSCALL(read);
    // Set up a simple readv system call.
    SYSCALL(readv);
    setup = 1;
}
