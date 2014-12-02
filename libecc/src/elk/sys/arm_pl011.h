/* Definitions for the ARM PL011 UART.
 * http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0183g/index.html
 */

#ifndef _arm_pl011_h_
#define _arm_pl011_h_

#include "config.h"
#include "pl011.h"              // RICH: One of these files has to go.

#define IRQ PL011_IRQ

#define UART(offset) ((volatile unsigned int *)((PL011_BASE + (offset))))

#define UARTDR     UART(0x000)  // Data register.
  #define DOE      0x0800       // Overrun error.
  #define DBE      0x0400       // Break error.
  #define DPE      0x0200       // Parity error.
  #define DFE      0x0100       // Framing error.
  #define DATA     0x00FF       // Transmit/receive data.

#define UARTRSR    UART(0x004)  // Receive status register.
#define UARTECR    UART(0x004)  // Error clear register.
  #define OE       0x0008       // Overrun error.
  #define BE       0x0004       // Break error.
  #define PE       0x0002       // Parity error.
  #define FE       0x0001       // Framing error.

#define UARTFR     UART(0x018)  // Flag register.
  #define RI       0x0100       // Ring indicator.   
  #define TXFE     0x0080       // Transmit FIFO empty.
  #define RXFF     0x0040       // Receive FIFO full.
  #define TXFF     0x0020       // Transmit FIFO full.
  #define RXFE     0x0010       // Receive FIFO empty.
  #define BUSY     0x0008       // UART busy.
  #define DCD      0x0004       // Data carrier detect.
  #define DSR      0x0002       // Data set ready.
  #define CTS      0x0001       // Clear to send.

#define UARTILPR   UART(0x020)  // IrDA low-power counter register.
#define UARTIBRD   UART(0x024)  // Integer baud rate register.
#define UARTFBRD   UART(0x028)  // Fractional baud rate register.

#define UARTCR_H   UART(0x02C)  // Line control register.
  #define SPS      0x0080       // Stick parity select.
  #define WLEN     0x0060       // Word length.
  #define FEN      0x0010       // Enable FIFOs.
  #define STP2     0x0008       // Two stop bits select.
  #define EPS      0x0004       // Even parity select.
  #define PEN      0x0002       // Parity enable.
  #define BRK      0x0001       // Send break.

#define UARTCR     UART(0x030)  // Control register.
  #define CTSEn    0x8000       // CTS hardware flow control enable.
  #define RTSEn    0x4000       // RTS hardware flow control enable.
  #define Out2     0x2000       // UART Out2.
  #define Out1     0x1000       // UART Out1.
  #define RTS      0x0800       // Request to send.
  #define DTR      0x0400       // Data transmit ready.
  #define RXE      0x0200       // Receive enable.
  #define TXE      0x0100       // Transmit enable.
  #define LBE      0x0080       // Loopback enable.
  #define SIRLP    0x0004       // SIR low-power IrDA mode.
  #define SIREN    0x0002       // SIR enable.
  #define UARTEN   0x0001       // UART enable.

#define UARTIFLS   UART(0x034)  // Interupt FIFO level select register.
  #define RXIFLSEL 0x0038       // Receive interrupt FIFO level select.
  #define TXIFLSEL 0x0007       // Transmit interrupt FIFO level select

#define UARTIMSC   UART(0x038)  // Interrupt mask register.
#define UARTRIS    UART(0x03C)  // Raw interrupt status register.
#define UARTMIS    UART(0x040)  // Masked interrupt status register.
#define UARTICR    UART(0x044)  // Interrupt clear register.
  #define OEI      0x0400       // Overrun error interrupt bit.
  #define BEI      0x0200       // Break error interrupt bit.
  #define PEI      0x0100       // Parity error interrupt bit.
  #define FEI      0x0080       // Framing error interrupt bit.
  #define RTI      0x0040       // Receive timeout interrupt bit.
  #define TXI      0x0020       // Transmit interrupt bit.
  #define RXI      0x0010       // Receive interrupt bit.
  #define DSRMI    0x0008       // nUARTDSR modem interrupt bit.
  #define DCDMI    0x0004       // nUARTDCD modem interrupt bit.
  #define CTSMI    0x0002       // nUARTCTS modem interrupt bit.
  #define RIMI     0x0002       // nUARTRI modem interrupt bit.

#define UARTDMACR  UART(0x048)  // DMA control register.
  #define DMAONERR 0x0004       // DMA on error.
  #define TXDMAE   0x0002       // Transmit DMA enable.
  #define RXDMAE   0x0001       // Receive DMA enable.

#endif // _arm_pl011_h_
