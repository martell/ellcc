#ifndef _irq_h_
#define _irq_h_

#include <inttypes.h>

#define IRQ_MAX_IDS     256     // The maximum number of interrupt IDs.

typedef struct irq_handler
{
    int id;                     // The interrupt identifier.
    int edge;                   // 1 = edge sensitive 0 = level.
    int priority;               // The priority level of the interruupt.
    int cpus;                   // A bit mask of CPUs to send the IRQ to.
    int sources;                // The number of sources in this vector.
    struct irq_entry {
        volatile uint32_t *irq_status;  // The interrupt status register.
        uint32_t irq_value;             // The interrupt active mask.
        volatile uint32_t *irq_clear;   // The interrupt clear register.
        uint32_t clear_value;           // The value to clear the interrupt.
        struct {
            void (*fn)(void *);         // The interrupt handler function.
            void **arg;                 // The handler argument pointer.
        } handler;
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
const void *__identify_irq(void);

/** Setup to handle an interrupt.
 * @param irq The interupptt number.
 * @param edge != 0 if edge sensitive.
 */
void irq_setup(const IRQHandler *handler);

/** Can we identify IRQs by ID?
 */
int irq_canid(void);

/** Get the interrupt ID and ack word of the current pending interrupt.
 */
int irq_getid(int *ack);

/** Acknowledge an interrupt.
 */
void irq_ack(int ack);

#endif // _irq_h_
