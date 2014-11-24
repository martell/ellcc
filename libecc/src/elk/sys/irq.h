/** Interrupt registration and handling.
 */
#ifndef _irq_h_
#define _irq_h_

#include <sys/types.h>
#include <inttypes.h>
#include <semaphore.h>

#include "config.h"

#if ELK_NAMESPACE
#define irq_attach __elk_irq_attach
#define irq_detach __elk_irq_detach
#define irq_handler __elk_irq_handler
#define irq_info __elk_irq_info
#define irq_init __elk_irq_init
#endif

typedef struct irq {
  int vector;                   // Vector number.
  int (*isr)(void *);           // Pointer to isr.
  void (*ist)(void *);          // Pointer to ist.
  void *data;                   // Data to be passed for ISR/IST.
  int priority;                 // Interrupt priority.
  u_int count;                  // Interrupt count.
  int istreq;                   // Number of ist request.
  pid_t  thread;                // Thread id of ist.
  sem_t istsem;                 // Event for ist.
} *irq_t;

// IRQ information.
struct irqinfo {
  int cookie;                   // Index cookie.
  int vector;                   // Vector number.
  u_int count;                  // Interrupt count.
  int priority;                 // Interrupt priority.
  int istreq;                   // Pending ist request.
  pid_t thread;                 // Thread id of ist.
};

// Map an interrupt priority level to IST priority.
#define ISTPRI(pri) (PRI_IST + (IPL_HIGH - pri))

irq_t irq_attach(int, int, int, int (*)(void *), void (*)(void *), void *);
void irq_detach(irq_t);
void irq_handler(int);
int irq_info(struct irqinfo *);
void irq_init(void);

// RICH: The rest may go away.

#define IRQ_MAX_IDS     256     // The maximum number of interrupt IDs.

typedef void (*InterruptFn)(void *);

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
            InterruptFn fn;             // The interrupt handler function.
            void **arg;                 // The handler argument pointer.
        } handler;
        uint32_t direct;        // != 0: Call the handler directly.
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
