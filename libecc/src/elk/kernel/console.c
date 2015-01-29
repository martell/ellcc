/* A simple interrupt driven console handler.
 */

#include <semaphore.h>
#include <unistd.h>
#include <syscalls.h>           // For syscall numbers.
#include <sys/uio.h>            // For writev (used by printf()).
#include <sys/ioctl.h>

#include "kernel.h"
#include "irq.h"
#include "console.h"
#include "crt1.h"

// Make the simple console a loadable feature.
FEATURE_CLASS(console, console)

/* The following input and output semaphores are used to insure
 * a the read, readv, write, and writev system calls complete
 * completely before another thread can do input or output.
 */
static sem_t sem_output;  // The output semaphore.
static sem_t sem_input;   // The input semaphore.

static sem_t sem_ibuffer; // The input buffer semaphore.
#define IBUFFER_SIZE 256
static char ibuffer[IBUFFER_SIZE];
static char *ibuffer_in;        // The input buffer input pointer.
static char *ibuffer_out;       // The input buffer output pointer.

static sem_t sem_obuffer; // The output buffer semaphore.
#define OBUFFER_SIZE 256
static char obuffer[OBUFFER_SIZE];
static char *obuffer_in;        // The output buffer input pointer.
static char *obuffer_out;       // The output buffer output pointer.

/** Handle a received character interrupt.
 */
static void rx_interrupt(void *arg)
{
  int ch = console_get_char_now();
  char *next = ibuffer_in + 1;
  if (next >= &ibuffer[IBUFFER_SIZE]) {
    next = ibuffer;
  }
  if (next == ibuffer_out) {
    // The buffer is full.
    return;                     // RICH: Deal with multiple buffers.
  }
  *ibuffer_in = ch;
  ibuffer_in = next;

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

/** Handle a transmiter empty interrupt.
 */
static void tx_interrupt(void *arg)
{
  // Check for a previously full buffer.
  char *next = obuffer_in + 1;
  if (next >= &obuffer[OBUFFER_SIZE]) {
    next = obuffer;
  }
  if (next == obuffer_out) {
    sem_post(&sem_obuffer);       // The buffer is not full any more.
  }

  next = obuffer_out + 1;
  if (next >= &obuffer[OBUFFER_SIZE]) {
    next = obuffer;
  }
  if (next == obuffer_in) {
    // The buffer is empty.
    // Disable the transmit interrupt.
    console_disable_tx_interrupt();
  }

  // Send the next character.
  console_send_char_now(*obuffer_out);
  obuffer_out = next;
}

/** Send the next character.
 */
static void send_char(int ch)
{
  /* Under QEMU the transmit FIFO is never full so for testing
   * just ignore it and set up for sending a character through
   * the output buffer.
   */
#undef TEST
#ifndef TEST
  if (console_send_char_nowait(ch)) {
    return;                     // The character was sent.
  }
#endif

  char *next = obuffer_in + 1;
  if (next >= &obuffer[OBUFFER_SIZE]) {
    next = obuffer;
  }
  if (next == obuffer_out) {
    // The buffer is full.
    sem_wait(&sem_obuffer);       // RICH: Deal with multiple buffers.
  }

  *obuffer_in = ch;
  obuffer_in = next;

#ifdef TEST
  /* Unfortunanately, an ARM pl011 UART will not assert a TXI interrupt
   * unless at least one character leaves it. Send a nul byte
   * to prime the pump.
   */
  console_send_char_now('\0');
#endif
  // Enable the transmit interrupt.
  console_enable_tx_interrupt();
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
    ++s;                        // Count the character.
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
      *p = '\n';                // and send a newline back.
      return s;                 // This read is done.
    default:
      send_char(ch);            // Echo input.
      *p++ = ch;                // and send it back.
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
    return 0;                   // Yes sir, we are a serial port.
  default:
    return -EINVAL;
  }
}

ELK_PRECONSTRUCTOR()
{
  static int setup;
  if (setup) {
    return;
  }

  // Set up the console for interrupt serial I/O.
  sem_init(&sem_output, 0, 1);
  sem_init(&sem_input, 0, 1);
  ibuffer_in = ibuffer_out = ibuffer;
  sem_init(&sem_obuffer, 0, 0);
  obuffer_in = obuffer_out = obuffer;

  // Set up the system calls.
  SYSCALL(ioctl);
  SYSCALL(write);
  SYSCALL(writev);
  SYSCALL(read);
  SYSCALL(readv);

  // Register the interrupt handler.
  console_interrupt_register(rx_interrupt, tx_interrupt);

  // Enable the receive interrupt.
  console_enable_rx_interrupt();
  setup = 1;
}
