/** The simplest hello world.
 */
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

static void init_serial() {
    *INTEN  = 0x00;             // Disable interrupts.
    *LCRTL  = 0x80;             // Set DLAB on.
    *RXTX   = 0x03;             // Set baud rate.
    *INTEN  = 0x00;
    *LCRTL  = 0x03;
    *IIFIFO = 0xc7;
    *MCRTL  = 0x0b;
}

void send_char(char a)
{
    while ((*LSTAT & 0x20) == 0);
    *RXTX = a;
}

static void printf(const char *s)
{
    while(*s != '\0') {
        send_char(*s++);
    }
}

void _startup()
{
    init_serial();

    printf("hello world\n");
    printf("type control-A x to get out of QEMU\n");
}
