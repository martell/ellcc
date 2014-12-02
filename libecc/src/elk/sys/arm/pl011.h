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
 * pl011.h - ARM PrimeCell PL011 UART
 */

#ifndef _pl011_h_
#define _pl011_h_

#include "config.h"

CONF_IMPORT(__pl011_physical_base__);
CONF_IMPORT(__pl011_base__);
CONF_IMPORT(__pl011_size__);
CONF_IMPORT(__pl011_irq__);
CONF_IMPORT(__pl011_clock__);
CONF_IMPORT(__pl011_baud_rate__);

#define PL011_PHYSICAL_BASE     CONF_ADDRESS(__pl011_physical_base__)
#define PL011_BASE              CONF_ADDRESS(__pl011_base__)
#define PL011_SIZE              CONF_ADDRESS(__pl011_size__)
#define PL011_IRQ               CONF_UNSIGNED(__pl011_irq__)
#define PL011_CLOCK             CONF_UNSIGNED(__pl011_clock__)
#define PL011_BAUD_RATE         CONF_UNSIGNED(__pl011_baud_rate__)

/* UART Registers */
#define UART_DR         (PL011_BASE + 0x00)
#define UART_RSR        (PL011_BASE + 0x04)
#define UART_ECR        (PL011_BASE + 0x04)
#define UART_FR         (PL011_BASE + 0x18)
#define UART_IBRD       (PL011_BASE + 0x24)
#define UART_FBRD       (PL011_BASE + 0x28)
#define UART_LCRH       (PL011_BASE + 0x2c)
#define UART_CR         (PL011_BASE + 0x30)
#define UART_IMSC       (PL011_BASE + 0x38)
#define UART_MIS        (PL011_BASE + 0x40)
#define UART_ICR        (PL011_BASE + 0x44)

// Flag register.
#define FR_RXFE         0x10    // Receive FIFO empty.
#define FR_TXFF         0x20    // Transmit FIFO full.

// Masked interrupt status register.
#define MIS_RX          0x10    // Receive interrupt.
#define MIS_TX          0x20    // Transmit interrupt.

// Interrupt clear register.
#define ICR_RX          0x10    // Clear receive interrupt.
#define ICR_TX          0x20    // Clear transmit interrupt.

// Line control register (High).
#define LCRH_WLEN8      0x60    // 8 bits.
#define LCRH_FEN        0x10    // Enable FIFO.

// Control register.
#define CR_UARTEN       0x0001  // UART enable.
#define CR_TXE          0x0100  // Transmit enable.
#define CR_RXE          0x0200  // Receive enable.

// Interrupt mask set/clear register.
#define IMSC_RX         0x10    // Receive interrupt mask.
#define IMSC_TX         0x20    // Transmit interrupt mask.

#endif // _pl011_h_
