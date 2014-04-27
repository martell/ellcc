/* Simple interrupt handling code.
 * Should be updated to handle vectors, etc.
 */
#include "irq.h"
#include "kernel.h"

static const IRQHandler *handlers[IRQ_MAX_IDS];
static int irq_index;

/** Register an interrupt handler.
 */
void irq_register(const IRQHandler *handler)
{
    handlers[irq_index] = handler;
    ++irq_index;

// RICH: Temporary. This is a hack until the PERIPH stuff is handled properly.
#define PERIPHBASE 0x1E000000
#define SCU     0x0000  // Snoop control unit.
#define GIC     0x0100  // Interrupt controller.
#define GTIMER  0x0200  // Global timer.
#define PTIMERS 0x0600  // Private timers and watchdogs.
#define IRQDIST 0x1000  // Interrupt distributor.

#define PREG(sec, reg) (*(unsigned int *)(address + (sec) + (reg)))
#define PADR(sec, reg) ((unsigned int *)((PERIPHBASE + (sec)  + (reg))))

static volatile unsigned char * const address = (unsigned char *)PERIPHBASE;

    int id = handler->irq + 32;
    int word = id / 32 * 4;
    // int bit = 1 << (id % 32);
    PREG(IRQDIST + 0x100, word) = 0xFFFFFFFF;
    word = id / 4 * 4;
    PREG(IRQDIST + 0x800, word) = 0x01010101;
    word = id / 16 * 4;
    PREG(IRQDIST + 0xC00, word) = 0;
    PREG(GIC, 0x4) = 0xFFFF;
    PREG(GIC, 0x0) = 0x3;
    PREG(IRQDIST, 0) = 0x01;
}

/** Identify an interrupt source, disable it, return a handler.
 */
void *__identify_irq(void)
{
    for (int i = 0; i < irq_index; ++i) {
        // Walk through all the handlers.
        const IRQHandler *hp = handlers[i];
        for (int j = 0; j < hp->sources; ++j) {
            // Check all the handler's entries.
            const struct irq_entry *ep = &hp->entries[j];
            if (*ep->irq_status & ep->irq_value) {
                *ep->irq_clear = ep->clear_value;
                return ep->handler;
            }
        }
    }
 
    // No interrupt handler found. Panic?
    return NULL;
}

