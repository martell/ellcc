/* Handle the exit system call.
 */
#include <bits/syscall.h>       // For syscall numbers.
#include <stdio.h>
#include <kernel.h>

// Make the simple console a loadable feature.
FEATURE(exit, exit)

static int sys_exit(int status)
{
    printf("The program has exited.\n");
    for( ;; )
      continue;
}

CONSTRUCTOR()
{
    // Set up a simple exit system call.
    __set_syscall(SYS_exit, sys_exit);
}
