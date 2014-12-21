/*-
 * Copyright (c) 2008, Kohsuke Ohtani
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
 * diag.c - diagnostic message support
 */

#include <stdarg.h>
#include <stdio.h>

#include "pl011.h"
#include "kernel.h"
#include "bootinfo.h"
#include "busio.h"
#include "vm.h"

#define UART_BASE PL011_BASE
#define UART_PHYSICAL_BASE PL011_PHYSICAL_BASE

// Flag register
#define FR_RXFE         0x10            // Receive FIFO empty.
#define FR_TXFF         0x20            // Transmit FIFO full.

static void serial_putc(int c)
{
  while (bus_read_32(UART_FR) & FR_TXFF)
    continue;

  bus_write_32(UART_DR, (uint32_t)c);
}

void diag_puts(char *buf)
{
  while (*buf) {
    if (*buf == '\n')
      serial_putc('\r');
    serial_putc(*buf++);
  }
}

static char buffer[CONFIG_DIAG_MSGSZ];

int diag_printf(const char *__restrict fmt, ...)
{
  va_list ap;

  int s = splhigh();
  va_start(ap, fmt);
  int c = vsnprintf(buffer, CONFIG_DIAG_MSGSZ, fmt, ap);

  diag_puts(buffer);
  va_end(ap);
  splx(s);
  return c;
}


void diag_init(void)
{
  // Map the UART physical address to its address in the kernel virtual space.
  vm_premap(UART_PHYSICAL_BASE, UART_BASE);
}
