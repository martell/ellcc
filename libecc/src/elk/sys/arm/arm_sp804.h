/* Definitions for the ARM SP804 Dual-Timer.
 * http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0271d/Babehiha.html
 */

#include "config.h"

CONF_IMPORT(__sp804_physical_base__);
CONF_IMPORT(__sp804_base__);
CONF_IMPORT(__sp804_size__);
CONF_IMPORT(__sp804_irq__);
CONF_IMPORT(__sp804_clock__);

#define SP804_BASE CONF_ADDRESS(__sp804_base__)
#define SP804_PHYSICAL_BASE CONF_ADDRESS(__sp804_physical_base__)
#define SP804_IRQ CONF_UNSIGNED(__sp804_irq__)
#define SP804_CLOCK CONF_UNSIGNED(__sp804_clock__)
#define SP804_SIZE CONF_SIZE(__sp804_clock__)

#define T(offset) ((volatile unsigned int *)((SP804_BASE + (offset))))

#define Timer1Load    T(0x000)  // Timer 1 load register.
#define Timer1Value   T(0x004)  // Current value register.
#define Timer1Control T(0x008)  // Timer 1 control register.
  #define TimerEn   0x0080      // 0 = disabled, 1 = enabled.
  #define TimerMode 0x0040      // 0 = free running, 1 = periodic.
  #define IntEnable 0x0020      // 0 = disabled, 1 = enabled.
  #define TimerPre  0x000C      // Prescale 00 = clk/1   01 = clk/16
                                //          01 = clk/256 11 = undefined
  #define TimerSize 0x0002      // 0 = 16-bit, 1 = 32-bit.
  #define OneShot   0x0001      // 0 = wrapping, 1 = one-shot.

#define Timer1IntClr  T(0x00C)  // Interrupt clear register.
#define Timer1RIS     T(0x010)  // Raw interrupt status register.
#define Timer1MIS     T(0x014)  // Masked interrupt status register.
  #define TimerInt  0x0001      // Timer interrupt bit.

#define Timer1BGLoad  T(0x018)  // Background load register.

#define Timer2Load    T(0x020)  // Timer 2 load register.
#define Timer2Value   T(0x024)  // Current value register.
#define Timer2Control T(0x028)  // Timer 2 control register.
#define Timer2IntClr  T(0x02C)  // Interrupt clear register.
#define Timer2RIS     T(0x030)  // Raw interrupt status register.
#define Timer2MIS     T(0x034)  // Masked interrupt status register.
#define Timer2BGLoad  T(0x038)  // Background load register.

