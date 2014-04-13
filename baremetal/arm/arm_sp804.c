/* Initialize the ARM SP804 dual timer.
 */

#include "kernel.h"
#include "arm_sp804.h"

static void init(void)
    __attribute__((__constructor__, __used__));

static void init(void)
{
    // Set up the timer.
}

