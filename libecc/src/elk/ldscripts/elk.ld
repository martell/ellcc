/* Define the target memory map. */
INCLUDE memory.ld
/* Configure the target environment. */
INCLUDE elkconfig.ld
/* Choose elk features to pull in. */
EXTERN(__elk_simple_console)
EXTERN(__elk_exit)
