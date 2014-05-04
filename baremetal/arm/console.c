/* A simple interrupt driven console handler.
 */

#include <semaphore.h>
#include <unistd.h>
#include <bits/syscall.h>       // For syscall numbers.
#include <sys/uio.h>            // For writev (used by printf()).
#include <sys/ioctl.h>

#include "kernel.h"
#include "arm_pl011.h"
#include "irq.h"

static sem_t sem_output;        // The output semaphore.
static sem_t sem_input;         // The input semaphore.
static sem_t sem_ibuffer;       // The input buffer semaphore.
#define IBUFFER_SIZE 256
static char ibuffer[IBUFFER_SIZE];
static char *ibuffer_in;        // The input buffer input pointer.
static char *ibuffer_out;       // The input buffer output pointer.

/** Handle a received character interrupt.
 */
static void rx_interrupt(void *arg)
{
    int ch = *UARTDR;
    *ibuffer_in++ = ch;
    if (ibuffer_in >= &ibuffer[IBUFFER_SIZE]) {
        ibuffer_in = ibuffer;
    }

    sem_post(&sem_ibuffer);
}

/** Get the next character from the input buffer.
 */
static int next_char(void)
{
    sem_wait(&sem_ibuffer);     // Wait for input.
    int ch = *ibuffer_out++;
    if (ibuffer_out >= &ibuffer[IBUFFER_SIZE]) {
        ibuffer_out = ibuffer;
    }
    return ch;
}

/** Send the next character.
 */
static void send_char(int ch)
{
    while (*UARTFR & TXFF)
        continue;           // Wait while TX FIFO is full.
    *UARTDR = ch;
}

static ssize_t do_write(int fd, const void *buf, size_t count)
{
    ssize_t s = 0;
    const unsigned char *p = buf;
    for ( ; s < count; ++s) {
        send_char(*p++);
    }
    return s;
}

static ssize_t sys_write(int fd, const void *buf, size_t count)
{
    int s = sem_wait(&sem_output);
    if (s < 0) return s;
    s = do_write(fd, buf, count);
    sem_post(&sem_output);
    return s;
}

static ssize_t sys_writev(int fd, const struct iovec *iov, int iovcount)
{
    ssize_t s = sem_wait(&sem_output);
    if (s < 0) return s;
    s = 0;
    for (int i = 0; i < iovcount; ++i) {
        ssize_t c = do_write(fd, iov[i].iov_base, iov[i].iov_len);
        if (c < 0) {
            s = c;
            break;
        }
        s += c;
    }
    sem_post(&sem_output);
    return s;
}

static ssize_t do_read(int fd, void *buf, size_t count)
{
    ssize_t s = 0;
    unsigned char *p = buf;
    for ( ; s < count; ) {
        int ch = next_char();
        ++s;                    // Count the character.
        switch (ch) {
            // Some simple line handling.
        case 0x7F:
        case '\b':
            // Simple backspace handling.
            --s;
            if (s) {
              send_char('\b');
              send_char(' ');
              send_char('\b');
              --p;
              --s;
            }
            break;
        case '\n':
        case '\r':
            // Make sure we send both a CR and LF.
            send_char(ch == '\r' ? '\n' : '\r');
            *p = '\n';          // and send a newline back.
            return s;           // This read is done.
        default:
            send_char(ch);      // Echo input.
            *p++ = ch;          // and send it back.
            break;
        }
    }
    return s;
}

static ssize_t sys_read(int fd, void *buf, size_t count)
{
    ssize_t s = sem_wait(&sem_input);
    if (s < 0) return s;
    s = do_read(fd, buf, count);
    sem_post(&sem_input);
    return s;
}

static ssize_t sys_readv(int fd, const struct iovec *iov, int iovcount)
{
    ssize_t s = sem_wait(&sem_input);
    if (s < 0) return s;
    s = 0;
    for (int i = 0; i < iovcount; ++i) {
        ssize_t c = do_read(fd, iov[i].iov_base, iov[i].iov_len);
        if (c < 0) {
            s = c;
            break;
        }
        s += c;
    }
    sem_post(&sem_input);
    return s;
}

static int sys_ioctl(int fd, int request, ...)
{
    switch (request) {
    case TCGETS:
        return 0;       // Yes sir, we are a serial port.
    default:
        return -EINVAL;
    }
}

static const IRQHandler serial_irq =
{
    .id = IRQ + 32,
    .edge = 0,
    .priority = 0,
    .cpus = 0xFFFFFFFF,         // Send to all CPUs.
    .sources = 1,
    {
        { UARTMIS, RXI, UARTICR, RXI,
            { rx_interrupt, NULL }},
#if RICH
        { UARTMIS, TXI, UARTICR, TXI,
            { tx_interrupt, NULL }},
#endif
    }
};

static void init(void)
    __attribute__((__constructor__, __used__));

static void init(void)
{
    // Set up the console for interrupt serial I/O.

    sem_init(&sem_output, 0, 1);
    sem_init(&sem_input, 0, 1);
    sem_init(&sem_ibuffer, 0, 0);
    ibuffer_in = ibuffer_out = ibuffer;
 
    // Set up a simple ioctl system call.
    __set_syscall(SYS_ioctl, sys_ioctl);
    // Set up a simple write system call.
    __set_syscall(SYS_write, sys_write);
    // Set up a simple writev system call.
    __set_syscall(SYS_writev, sys_writev);
    // Set up a simple read system call.
    __set_syscall(SYS_read, sys_read);
    // Set up a simple readv system call.
    __set_syscall(SYS_readv, sys_readv);

    // Register the interrupt handler.
    irq_register(&serial_irq);

    // Enable the receive interrupt.
    *UARTIMSC = RXI;
}
