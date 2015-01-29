/*-
 * Copyright (c) 2008-2009, Kohsuke Ohtani
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * pl011.c - ARM PrimeCell PL011 UART
 */

#include "hal.h"
#include "ipl.h"
#include "irq.h"
#include "tty.h"
#include "serial.h"
#include "busio.h"
#include "pl011.h"

// Make the pl011 driver a loadable feature.
FEATURE(pl011_dev)

/* #define DEBUG_PL011 1 */

// Forward functions.
static int pl011_init(struct driver *);
static void pl011_xmt_char(struct serial_port *, int);
static int pl011_rcv_char(struct serial_port *);
static void pl011_set_poll(struct serial_port *, int);
static void pl011_start(struct serial_port *);
static void pl011_stop(struct serial_port *);

static struct driver pl011_driver = {
  "pl011",      // name
  NULL,         // devops
  0,            // devsz
  0,            // flags
  NULL,         // probe
  pl011_init,   // init
  NULL,         // detatch
};

static struct serial_ops pl011_ops = {
  pl011_xmt_char,
  pl011_rcv_char,
  pl011_set_poll,
  pl011_start,
  pl011_stop,
};

static struct serial_port pl011_port;

static void pl011_xmt_char(struct serial_port *sp, int c)
{
  while (bus_read_32(UART_FR) & FR_TXFF)
    continue;
  bus_write_32(UART_DR, (uint32_t)c);
  // Enable TX/RX interrupt.
  bus_write_32(UART_IMSC, (IMSC_RX | IMSC_TX));
}

static int pl011_rcv_char(struct serial_port *sp)
{
  while (bus_read_32(UART_FR) & FR_RXFE)
    continue;
  return bus_read_32(UART_DR) & 0xFF;
}

static void pl011_set_poll(struct serial_port *sp, int on)
{
  if (on) {
     // Disable interrupt for polling mode.
    bus_write_32(UART_IMSC, 0);
  } else {
    bus_write_32(UART_IMSC, (IMSC_RX | IMSC_TX));
  }
}

static int pl011_isr(void *arg)
{
  struct serial_port *sp = arg;

  uint32_t mis = bus_read_32(UART_MIS);
  if (mis & MIS_RX) {
    // Receive interrupt.
    while (bus_read_32(UART_FR) & FR_RXFE)
      continue;

    do {
      int c = bus_read_32(UART_DR);
      serial_rcv_char(sp, c);
    } while ((bus_read_32(UART_FR) & FR_RXFE) == 0);

    // Clear interrupt status.
    bus_write_32(UART_ICR, ICR_RX);
  }

  if (mis & MIS_TX) {
    // Transmit interrupt.
    // Clear interrupt status.
    bus_write_32(UART_ICR, ICR_TX);
    // Disable the TX interrupt.
    bus_write_32(UART_IMSC, IMSC_RX);
    serial_xmt_done(sp);
  }

  return 0;
}

static void pl011_start(struct serial_port *sp)
{
  uint32_t divider, remainder, fraction;

  bus_write_32(UART_CR, 0);             // Disable everything.
  bus_write_32(UART_ICR, 0x07FF);       // Clear all interrupt status.

  /** Set baud rate:
   * IBRD = PL011_CLOCK / (16 * PL011_BAUD_RATE)
   * FBRD = ROUND((64 * MOD(PL011_CLOCK,(16 * PL011_BAUD_RATE)))
   *        / (16 * PL011_BAUD_RATE))
   */
  divider = PL011_CLOCK / (16 * PL011_BAUD_RATE);
  remainder = PL011_CLOCK % (16 * PL011_BAUD_RATE);
  fraction = (8 * remainder / PL011_BAUD_RATE) >> 1;
  fraction += (8 * remainder / PL011_BAUD_RATE) & 1;
  bus_write_32(UART_IBRD, divider);
  bus_write_32(UART_FBRD, fraction);

  // Set N, 8, 1, FIFO enable.
  bus_write_32(UART_LCRH, (LCRH_WLEN8|LCRH_FEN));

  // Enable UART.
  bus_write_32(UART_CR, (CR_RXE|CR_TXE|CR_UARTEN));

#if 1   // RICH
    static IRQHandler serial_irq =
    {
        .id = PL011_IRQ + 32,
        .edge = 0,
        .priority = IPL_COMM,
        .cpus = 0xFFFFFFFF,         // Send to all CPUs.
        .sources = 1,
        {
            { (void *)UART_MIS, MIS_TX|MIS_RX, (void *)UART_ICR, MIS_TX|MIS_RX,
                { .fn = (void *)pl011_isr },
              .direct = 1,
            },
        }
    };
    serial_irq.entries[0].handler.arg = (void *)sp;
    irq_register(&serial_irq);

#else
  // Install interrupt handler.
  sp->irq = irq_attach(UART_IRQ, IPL_COMM, 0, pl011_isr, IST_NONE, sp);
#endif

  // Enable RX interrupt.
  bus_write_32(UART_IMSC, IMSC_RX);
}

static void pl011_stop(struct serial_port *sp)
{
  bus_write_32(UART_IMSC, 0);   // Disable all interrupts.
  bus_write_32(UART_CR, 0);     // Disable everything.
}

static int pl011_init(struct driver *self)
{

  serial_attach(&pl011_ops, &pl011_port);
  return 0;
}

ELK_PRECONSTRUCTOR()
{
  driver_register(&pl011_driver);
}
