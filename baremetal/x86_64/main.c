/* A simple bare metal main.
 */

#include <stdio.h>
#include <command.h>

int main(int argc, char **argv)
{
    puts("type control-A x to get out of QEMU");
    printf("%s started. Type \"help\" for a list of commands.\n", argv[0]);
    for ( ;; ) 
        continue;
    // Enter the kernel command processor.
    // do_commands(argv[0]);
}
