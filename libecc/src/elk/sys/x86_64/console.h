#ifndef _console_h_
#define _console_h_

#include "io.h"
#include "irq.h"

#define UART0_BASE 0x3f8
#define UART_REG(n) ((UART0_BASE) + ((n)))
#define RXTX    UART_REG(0)
#define INTEN   UART_REG(1)
#define IIFIFO  UART_REG(2)
#define LCRTL   UART_REG(3)
#define MCRTL   UART_REG(4)
#define LSTAT   UART_REG(5)
#define MSTAT   UART_REG(6)
#define SCRATCH UART_REG(7)

#if RICH
static void init() {
    outb(0x00, INTEN);                  // Disable interrupts.
    outb(0x80, LCRTL);                  // Set DLAB on.
    outb(0x03, RXTX);                   // Set baud rate.
    outb(0x00, INTEN);
    outb(0x03, LCRTL);
    outb(0xC7, IIFIFO);
    outb(0x0B, MCRTL);
}
#endif

#ifdef SIMPLE_CONSOLE

/** Send a character to the serial port.
 */
static void console_send_char(int ch)
{
    while ((inb(LSTAT) & 0x20) == 0)
        continue;           // Wait while TX FIFO is not empty.
    outb(ch, RXTX);
}

/** Get a character from the serial port.
 */
static int console_get_char(void)
{
    while ((inb(LSTAT) & 0x01) == 0)
        continue;           // Wait while RX FIFO is empty.
    return inb(RXTX);
}

#else

/** Send a character to the serial port. The transmit buffer is empty.
 */
static inline void console_send_char_now(int ch)
{
#if RICH
    *UARTDR = ch;
#endif
}

/** Send a character to the serial port if the transmit buffer is empty.
 */
static inline int console_send_char_nowait(int ch)
{
#if RICH
    if ((*UARTFR & TXFF) == 0) {
        // The transmit buffer is empty. Send the character.
        *UARTDR = ch;
        return 1;
    }

#endif
    return 0;
}

/** Get a character from the serial port. The receive buffer is full.
 */
static inline int console_get_char_now(void)
{
#if RICH
    return *UARTDR;
#else
    return 0;
#endif
}

/** Enable the transmit interrupt.
 */
static inline void console_enable_tx_interrupt(void)
{
#if RICH
    *UARTIMSC |= TXI;
#endif
}

/** Disable the transmit interrupt.
 */
static inline void console_disable_tx_interrupt(void)
{
#if RICH
    *UARTIMSC &= ~TXI;
#endif
}

/** Enable the receive interrupt.
 */
static inline void console_enable_rx_interrupt(void)
{
#if RICH
    *UARTIMSC = RXI;
#endif
}

/** Register the console interrupt handler.
 */
static void console_interrupt_register(InterruptFn rx, InterruptFn tx)
{
#if RICH
    static IRQHandler serial_irq =
    {
        .id = IRQ + 32,
        .edge = 0,
        .priority = 0,
        .cpus = 0xFFFFFFFF,         // Send to all CPUs.
        .sources = 2,
        {
            { UARTMIS, RXI, UARTICR, RXI,
                { NULL, NULL }},
            { UARTMIS, TXI, UARTICR, TXI,
                { NULL, NULL }},
        }
    };

    serial_irq.entries[0].handler.fn = rx;
    serial_irq.entries[1].handler.fn = tx;
    irq_register(&serial_irq);
#endif
}

#endif // SIMPLE_CONSOLE
#endif // _console_h_
