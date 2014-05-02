/** The ARM Generic Interrupt Controller.
 */

#include "arm_gic.h"
#include "irq.h"

/** Setup to handle an interrupt.
 * @param irq The interupptt number.
 * @param edge != 0 if edge sensitive.
 */
void irq_setup(const IRQHandler *handler)
{
    int id = handler->irq + 32;
    int mask, value;

    *ICDDCR = 0x00;             // Disable the CPU interface and distributer.
 
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

    // Set the processor(s)s to send the interrupt to.
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
