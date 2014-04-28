/* A simple bare metal main.
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>
#include "command.h"

int main(int argc, char **argv)
{
    printf("%s started. Type \"help\" for a list of commands\n", argv[0]);
    do_commands(argv[0]);
}
