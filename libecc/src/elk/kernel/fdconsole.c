/* A console handler that works through file descriptors.
 */

#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <syscalls.h>           // For syscall numbers.
#include <sys/uio.h>            // For writev (used by printf()).
#include <sys/ioctl.h>
#include "kernel.h"
#include "irq.h"
#include "console.h"
#include "file.h"
#include "vnode.h"
#include "device.h"
#include "thread.h"

// Make the simple console a loadable feature.
FEATURE_CLASS(fdconsole, console)

/* The following input and output semaphores are used to insure
 * a the read, readv, write, and writev system calls complete
 * completely before another thread can do input or output.
 */
static sem_t sem_output;        // The output semaphore.
static sem_t sem_input;         // The input semaphore.

static sem_t sem_ibuffer;       // The input buffer semaphore.
#define IBUFFER_SIZE 256
static char ibuffer[IBUFFER_SIZE];
static char *ibuffer_in;        // The input buffer input pointer.
static char *ibuffer_out;       // The input buffer output pointer.

static sem_t sem_obuffer;       // The output buffer semaphore.
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
  sem_wait(&sem_ibuffer);               // Wait for input.
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
    sem_post(&sem_obuffer);             // The buffer is not full any more.
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
    return;                             // The character was sent.
  }
#endif

  char *next = obuffer_in + 1;
  if (next >= &obuffer[OBUFFER_SIZE]) {
    next = obuffer;
  }
  if (next == obuffer_out) {
    // The buffer is full.
    sem_wait(&sem_obuffer);             // RICH: Deal with multiple buffers.
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

static ssize_t do_write(const void *buf, size_t count)
{
  ssize_t s = 0;
  const unsigned char *p = buf;
  for ( ; s < count; ++s) {
    send_char(*p++);
  }
  return s;
}

static int con_write(vnode_t vnode, file_t file, struct uio *uio, size_t *size)
{
  int ss = sem_wait(&sem_output);
  if (ss < 0) return ss;
  size_t s = 0;
  for (int i = 0; i < uio->iovcnt; ++i) {
      ssize_t c = do_write(uio->iov[i].iov_base, uio->iov[i].iov_len);
      if (c < 0) {
          s = c;
          break;
      }
      s += c;
  }

  sem_post(&sem_output);
  *size = s;
  return 0;
}

static ssize_t do_read(void *buf, size_t count)
{
  ssize_t s = 0;
  unsigned char *p = buf;
  for ( ; s < count; ) {
    int ch = next_char();
    ++s;                                // Count the character.
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
      *p = '\n';                        // and send a newline back.
      return s;                         // This read is done.
    default:
      send_char(ch);                    // Echo input.
      *p++ = ch;                        // and send it back.
      break;
    }
  }
  return s;
}

static int con_read(vnode_t vnode, file_t file, struct uio *uio, size_t *size)
{
  int ss = sem_wait(&sem_input);
  if (ss < 0) {
    return ss;
  }
  size_t s = 0;
  for (int i = 0; i < uio->iovcnt; ++i) {
    ssize_t c = do_read(uio->iov[i].iov_base, uio->iov[i].iov_len);
    if (c < 0) {
      s = c;
      break;
    }
    s += c;
  }
  sem_post(&sem_input);
  *size = s;
  return 0;
}

static int con_ioctl(vnode_t vnode, file_t file, unsigned long cmd, void *arg)
{
  switch (cmd) {
  case TCGETS:
    return 0;                           // Yes sir, we are a serial port.
  default:
    return -EINVAL;
  }
}

// RICH: This is very temporary.
#define con_mount ((vfsop_mount_t)vfs_nullop)
#define con_unmount ((vfsop_umount_t)vfs_nullop)
#define con_sync ((vfsop_sync_t)vfs_nullop)
#define con_vget ((vfsop_vget_t)vfs_nullop)
#define con_statfs ((vfsop_statfs_t)vfs_nullop)
#define con_open ((vnop_open_t)vop_nullop)
#define con_close ((vnop_close_t)vop_nullop)
static int con_read(vnode_t, file_t, struct uio *, size_t *);
static int con_write(vnode_t, file_t, struct uio *, size_t *);
#define con_poll ((vnop_poll_t)vop_nullop)
#define con_seek  ((vnop_seek_t)vop_nullop)
static int con_ioctl  (vnode_t, file_t, u_long, void *);
#define con_fsync  ((vnop_fsync_t)vop_nullop)
#define con_readdir  ((vnop_readdir_t)vop_nullop)
#define con_lookup  ((vnop_lookup_t)vop_nullop)
#define con_create  ((vnop_create_t)vop_einval)
#define con_remove  ((vnop_remove_t)vop_einval)
#define con_rename  ((vnop_rename_t)vop_einval)
#define con_mkdir  ((vnop_mkdir_t)vop_einval)
#define con_rmdir  ((vnop_rmdir_t)vop_einval)
#define con_getattr  ((vnop_getattr_t)vop_nullop)
#define con_setattr  ((vnop_setattr_t)vop_nullop)
#define con_inactive  ((vnop_inactive_t)vop_nullop)
#define con_truncate  ((vnop_truncate_t)vop_nullop)

struct vnops vnops = {
  con_open,           /* open */
  con_close,          /* close */
  con_read,           /* read */
  con_write,          /* write */
  con_poll,           /* poll */
  con_seek,           /* seek */
  con_ioctl,          /* ioctl */
  con_fsync,          /* fsync */
  con_readdir,        /* readdir */
  con_lookup,         /* lookup */
  con_create,         /* create */
  con_remove,         /* remove */
  con_rename,         /* remame */
  con_mkdir,          /* mkdir */
  con_rmdir,          /* rmdir */
  con_getattr,        /* getattr */
  con_setattr,        /* setattr */
  con_inactive,       /* inactive */
  con_truncate,       /* truncate */
};

struct vnode vnode = {
  .v_mount = NULL,              // Mounted vfs pointer.
  .v_op = &vnops,               // Vnode operations.
  .v_refcnt = 1,                // Reference count.
  .v_type = VCHR,               // Vnode type.
  .v_flags = 0,                 // Vnode flags.
  .v_mode = VREAD|VWRITE,       // File mode.
  .v_size = 0,                  // File size.
  .v_interlock = PTHREAD_MUTEX_INITIALIZER, // Lock for this vnode.
  .v_nrlocks = 0,               // Lock count.
  .v_blkno = 0,                 // Block number.
  .v_path = "/tty",             // Pointer to path in fs.
  .v_data = NULL,               // Private data for fs.
};

struct file file = {
  .f_offset = 0,
  .f_flags = FREAD|FWRITE,
  .f_count = 1,
  .f_vnode = &vnode,
};

int __elk_fdconsole_open(fdset_t *fdset)
{
  // Create a file descriptor for the console.
  sem_init(&vnode.v_wait, 0, 0);
  return allocfd(&file);
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

  // Register the interrupt handler.
  console_interrupt_register(rx_interrupt, tx_interrupt);

  // Enable the receive interrupt.
  console_enable_rx_interrupt();

  setup = 1;
}

C_CONSTRUCTOR()
{
#if RICH
  // Set up console device.
  static const struct devops devops = {
  };
  static const struct driver drv = {
    .name = "tty",
    .devops = &devops;
  };
  __elk_device_create(&drv, "tty", D_CHR);
#endif
}
