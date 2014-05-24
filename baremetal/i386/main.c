/* A simple bare metal main.
 */

#include <stdio.h>
#include <command.h>

#define SIMPLE_CONSOLE
#include "console.h"
int main(int argc, char **argv)
{
    extern void apic_init(void);
    apic_init();
    init();
    puts("type control-A x to get out of QEMU");
    printf("%s started. Type \"help\" for a list of commands.\n", argv[0]);
    // Enter the kernel command processor.
    do_commands(argv[0]);
}
