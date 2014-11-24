/* Interrupt handling code.
 */
#include <pthread.h>
#include "irq.h"

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static const IRQHandler *handlers[IRQ_MAX_IDS];
static int irq_index;

/** Register an interrupt handler.
 */
void irq_register(const IRQHandler *handler)
{
  pthread_mutex_lock(&mutex);
  if (irq_canid()) {
    // Interrupts are identified by the hardware.
    // Add to the list by ID.
    handlers[handler->id] = handler;
  } else {
    // Interrupt sources must be found.
    // Keep a sequential list.
    handlers[irq_index] = handler;
    ++irq_index;
  }

  // Set up the IRQ hardware.
  irq_setup(handler);
  pthread_mutex_unlock(&mutex);
}

/** Identify an interrupt source, disable it, return a handler.
 * This is called from the interrupt handler in init.S.
 */
const void *__elk_identify_irq(void)
{
  int ack;
  int id = irq_getid(&ack);           // Get the interrupt ID and ack.
  if (id >= 0) {
    // We have a valid ID.
    const IRQHandler *hp = handlers[id];
    for (int i = 0; i < hp->sources; ++i) {
      // Check all the handler's entries.
      const struct irq_entry *ep = &hp->entries[i];
      if (*ep->irq_status & ep->irq_value) {
        if (ep->direct) {
          // RICH: Call the interrupt handler directly.
          ep->handler.fn(ep->handler.arg);
          irq_ack(ack);         // Acknowledge the interrupt.
          return NULL;
        }
        *ep->irq_clear = ep->clear_value;
        irq_ack(ack);           // Acknowledge the interrupt.
        return &ep->handler;
      }
    }
  } else {
    // Search for the interrupt source.
    for (int i = 0; i < irq_index; ++i) {
      // Walk through all the handlers.
      const IRQHandler *hp = handlers[i];
      for (int j = 0; j < hp->sources; ++j) {
        // Check all the handler's entries.
        const struct irq_entry *ep = &hp->entries[j];
        if (*ep->irq_status & ep->irq_value) {
          *ep->irq_clear = ep->clear_value;
          return &ep->handler;
        }
      }
    }
  }

  // No interrupt handler found. Panic?
  return NULL;
}
