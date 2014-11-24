/** The ARM Generic Interrupt Controller.
 */

#include "hal.h"
#include "arm_priv.h"
#include "irq.h"

// RICH: Stubs for now.

/** Unmask interrupt in ICU for specified irq.
 * The interrupt mask table is also updated.
 * Assumes CPU interrupt is disabled in caller.
 */
void interrupt_unmask(int vector, int level)
{
}

/** Setup interrupt mode.
 * Select whether an interrupt trigger is edge or level.
 */
void interrupt_setup(int vector, int mode)
{
  // nop
}

/** Can we identify IRQs by ID?
 */
int irq_canid(void)
{
    return 1;           // Yes.
}

/** Get the interrupt ID of the current pending interrupt.
 */
int irq_getid(int *ack)
{
    int id = *ICCIAR;
    *ack = id;                  // The acknowledge word.
    id &= 0x3FF;                // Trim off the ID bits.
    if (id == 0x3FF) {
        return -1;
    }

    return id;
}

/** Acknowledge an interrupt.
 */
void irq_ack(int ack)
{
    *ICCEOIR = ack;
}

/** Setup to handle an interrupt.
 * @param handler The IRQ handler descriptor.
 */
void irq_setup(const IRQHandler *handler)
{
    int id = handler->id;
    int mask, value;

    *ICDDCR = 0x00;             // Disable the CPU interface and distributor.

    // Set level or edge sensitive.
    mask = 0x3 << (id % 16);
    value = (handler->edge ? 0x2 : 0x0) << (id % 16);
    ICDICFRn[id / 16] &= ~mask;
    ICDICFRn[id / 16] |= value;

    // Set the priority.
    mask = 0xFF << (id % 4);
    value = (handler->priority & 0xFF) << (id % 4);
    ICDIPRn[id / 4] &= ~mask;
    ICDIPRn[id / 4] |= value;

    // Set the processor(s) to which to send the interrupt.
    mask = 0xFF << (id % 4);
    value = (handler->cpus & 0xFF) << (id % 4);
    ICDIPTRn[id / 4] &= ~mask;
    ICDIPTRn[id / 4] |= value;
    
    // Enable the interrupt.
    ICDISERn[id / 32] = 1 << (id % 32);

    // Set the interrupt mask register. (No interrupts masked.)
    *ICCPMR = 0xFF;

    // Set the interface control register.
    *ICCICR = 0x3;              // Enable secure and non-secure handling.
    *ICDDCR = 0x01;             // Enable the CPU interface and distributer.
}
