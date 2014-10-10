#ifndef _console_h_
#define _console_h_

#include "irq.h"

#define BASE_ADDRESS 0xB8000000
#define UART0_BASE (BASE_ADDRESS + 0x3f8)
#define UART_REG(n) ((volatile char *)((UART0_BASE) + ((n))))
#define RXTX    UART_REG(0)
#define INTEN   UART_REG(1)
#define IIFIFO  UART_REG(2)
#define LCRTL   UART_REG(3)
#define MCRTL   UART_REG(4)
#define LSTAT   UART_REG(5)
#define MSTAT   UART_REG(6)
#define SCRATCH UART_REG(7)

// RICH: Put this here until proper UART support is inplemented.
static void init(void)
    __attribute__((__constructor__, __used__));

static void init(void)
{
    *INTEN  = 0x00;             // Disable interrupts.
    *LCRTL  = 0x80;             // Set DLAB on.
    *RXTX   = 0x03;             // Set baud rate.
    *INTEN  = 0x00;
    *LCRTL  = 0x03;
    *IIFIFO = 0xc7;
    *MCRTL  = 0x0b;
}

#ifdef SIMPLE_CONSOLE
/** Send a character to the serial port.
 */
static void console_send_char(int ch)
{
    while ((*LSTAT & 0x20) == 0)
        continue;           // Wait while TX FIFO is not empty.
    *RXTX = ch;
}

/** Get a character from the serial port.
 */
static int console_get_char(void)
{
    while ((*LSTAT & 0x01) == 0)
        continue;           // Wait while RX FIFO is empty.
    return *RXTX;
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
