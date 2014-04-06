/*******************************************/
/* Simple Bare metal program init */
/*******************************************/

/* Note: QEMU model of PL011 serial port ignores the transmit
 * FIFO capabilities. When writing on a real SOC, the
 * "Transmit FIFO Full" flag must be checked in UARTFR register
 * before writing on the UART register*/

#include <bits/syscall.h>

#define UART0_BASE 0xA80003f8  /* 8250 COM1 */
#define UART_REG(n) ((volatile char *)((UART0_BASE) + ((n) /* * 8 */)))
#define RXTX    UART_REG(0)
#define INTEN   UART_REG(1)
#define IIFIFO  UART_REG(2)
#define LCRTL   UART_REG(3)
#define MCRTL   UART_REG(4)
#define LSTAT   UART_REG(5)
#define MSTAT   UART_REG(6)
#define SCRATCH UART_REG(7)

static void init_serial() {
    *INTEN  = 0x00;             // Disable interrupts.
    *LCRTL  = 0x80;             // Set DLAB on.
    *RXTX   = 0x03;             // Set baud rate.
    *INTEN  = 0x00;
    *LCRTL  = 0x03;
    *IIFIFO = 0xc7;
    *MCRTL  = 0x0b;
}

void write_serial(char a)
{
    while ((*LSTAT & 0x20) == 0);
    *RXTX = a;
}

static void uart_print(const char *s)
{
    while(*s != '\0') {
        write_serial(*s++);
    }
}

/* Main entry point */
void _startup()
{
    init_serial();
    write(0, "foo\n", sizeof("foo\n"));

    uart_print("Welcome to Simple bare-metal program\n");
    uart_print("If you're running in QEMU, press Ctrl+a\n");
    uart_print("and then x to stop me...\n");
}


