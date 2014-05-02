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
    // Set up the IRQ hardware.
    irq_setup(handler);
    ++irq_index;
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

