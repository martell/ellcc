#ifndef _arm_h_
#define _arm_h_

#define Mode_USR    0x10
#define Mode_FIQ    0x11
#define Mode_IRQ    0x12
#define Mode_SVC    0x13
#define Mode_ABT    0x17
#define Mode_UND    0x1B
#define Mode_SYS    0x1F

#define T_bit 0x20
#define F_bit 0x40
#define I_bit 0x80


#if !defined(__ASSEMBLER__)
static inline uint32_t __update_cpsr(uint32_t clear, uint32_t eor) __attribute__((__unused__));

static inline uint32_t __update_cpsr(uint32_t clear, uint32_t set)
{
    uint32_t       old, new;

    asm volatile("mrs   %0, cpsr\n"     // Get the cpsr.
                 "bic   %1, %0, %2\n"   // Clear the affected bits.
                 "eor   %1, %1, %3\n"   // Set the desited bits.
                 "msr   cpsr_c, %1\n"   // Update the cpsr
                                        // The old value is in r0.
                 : "=&r" (old), "=&r" (new) : "r" (clear), "r" (set) : "memory");
    
    return old;
}

/** Turn off all interrupts.
 * @return The current interrupt level.
 */
static inline int splhigh(void)
{
    return __update_cpsr(F_bit | I_bit, F_bit | I_bit);
}

/** Turn on all interrupts.
 * @return The current interrupt level.
 */
static inline int spl0(void)
{
    return __update_cpsr(F_bit | I_bit, 0);
}

/** Set the interrupt level.
 * @param level The level to set.
 */
static inline void splx(int s)
{
    __update_cpsr(F_bit | I_bit, s & (F_bit | I_bit));
}
#endif // !defined(__ASSEMBLER__)

#endif // _arm_h_
