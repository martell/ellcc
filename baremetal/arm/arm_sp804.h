/* Definitions for the ARM SP804 Dual-Timer.
 * http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0271d/Babehiha.html
 */
#define VEXPRESS_A9
#if defined (VERSATILEPB)
#define BASE 0x0101E2000
#elif defined (VEXPRESS_A9)
#define BASE 0x10011000
#define CLOCK 45000000
#else // Newer cores.
#define BASE 0x1C110000
#define CLOCK 40000000
#endif

#define REG(reg) (*(unsigned int *)(address + (reg)))
#define ADR(reg) ((unsigned int *)(BASE + (reg)))

static volatile unsigned char * const address = (unsigned char *)BASE;

#define Timer1Load      0x000   // Timer 1 load register.
#define Timer1Value     0x004   // Current value register.
#define Timer1Control   0x008   // Timer 1 control register.
  #define TimerEn   0x0080      // 0 = disabled, 1 = enabled.
  #define TimerMode 0x0040      // 0 = free running, 1 = periodic.
  #define IntEnable 0x0020      // 0 = disabled, 1 = enabled.
  #define TimerPre  0x000C      // Prescale 00 = clk/1   01 = clk/16
                                //          01 = clk/256 11 = undefined
  #define TimerSize 0x0002      // 0 = 16-bit, 1 = 32-bit.
  #define OneShot   0x0001      // 0 = wrapping, 1 = one-shot.

#define Timer1IntClr    0x00C   // Interrupt clear register.
#define Timer1RIS       0x010   // Raw interrupt status register.
#define Timer1MIS       0x014   // Masked interrupt status register.
  #define TimerInt  0x0001      // Timer interrupt bit.

#define Timer1BGLoad    0x018   // Background load register.

#define Timer2Load      0x020   // Timer 2 load register.
#define Timer2Value     0x024   // Current value register.
#define Timer2Control   0x028   // Timer 2 control register.
#define Timer2IntClr    0x02C   // Interrupt clear register.
#define Timer2RIS       0x030   // Raw interrupt status register.
#define Timer2MIS       0x034   // Masked interrupt status register.
#define Timer2BGLoad    0x038   // Background load register.

