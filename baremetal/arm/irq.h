#ifndef _irq_h_
#define _irq_h_

#include <inttypes.h>

#define IRQ_MAX_IDS     128     // The maximum number of interrupt IDs.

typedef struct irq_handler
{
    int irq;                    // The interrupt identifier.
    int sources;                // The number of sources in this vector.
    struct irq_entry {
        volatile uint32_t *irq_status;  // The interrupt status register.
        uint32_t irq_value;             // The interrupt active mask.
        volatile uint32_t *irq_clear;   // The interrupt clear register.
        uint32_t clear_value;           // The value to clear the interrupt.
        void (*handler)();              // The interrupt handler funcrion.
        void *unused1;
        void *unused2;
        void *unused3;
    } entries[];
} IRQHandler;

/** Register an interrupt handler.
 */
void irq_register(const IRQHandler *handler);

/** Identify an interrupt source, disable it, return a handler.
 * This is called from the interrupt handler in init.S.
 */
void *__identify_irq(void);

#endif // _irq_h_
