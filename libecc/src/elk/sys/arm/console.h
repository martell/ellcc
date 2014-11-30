/** Console definitions specific to the ARM Pl011 UART.
 */
#ifndef _console_h_
#define _console_h_

#include "arm_pl011.h"
#include "irq.h"

#ifdef SIMPLE_CONSOLE
/** Send a character to the serial port.
 */
static void console_send_char(int ch)
{
    while (*UARTFR & TXFF)
        continue;           // Wait while TX FIFO is full.
    *UARTDR = ch;
}

/** Get a character from the serial port.
 */
static int console_get_char(void)
{
    while (*UARTFR & RXFE)
        continue;           // Wait while RX FIFO is empty.
    return *UARTDR;
}

#else

/** Send a character to the serial port. The transmit buffer is empty.
 */
static inline void console_send_char_now(int ch)
{
    *UARTDR = ch;
}

/** Send a character to the serial port if the transmit buffer is empty.
 */
static inline int console_send_char_nowait(int ch)
{
    if ((*UARTFR & TXFF) == 0) {
        // The transmit buffer is empty. Send the character.
        *UARTDR = ch;
        return 1;
    }

    return 0;
}

/** Get a character from the serial port. The receive buffer is full.
 */
static inline int console_get_char_now(void)
{
    return *UARTDR;
}

/** Enable the transmit interrupt.
 */
static inline void console_enable_tx_interrupt(void)
{
    *UARTIMSC |= TXI;
}

/** Disable the transmit interrupt.
 */
static inline void console_disable_tx_interrupt(void)
{
    *UARTIMSC &= ~TXI;
}

/** Enable the receive interrupt.
 */
static inline void console_enable_rx_interrupt(void)
{
    *UARTIMSC = RXI;
}

/** Register the console interrupt handler.
 */
#include "bootinfo.h"
static void console_interrupt_register(InterruptFn rx, InterruptFn tx)
{
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
}

#endif // SIMPLE_CONSOLE

#endif // _console_h_
