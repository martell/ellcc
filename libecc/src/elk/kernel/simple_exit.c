/* Handle the exit system call.
 */
#include <bits/syscall.h>       // For syscall numbers.
#include <stdio.h>
#include <kernel.h>

// Make simple exit a loadable feature.
FEATURE(simple_exit, exit)

static int sys_exit(int status)
{
    for( ;; )
      continue;
}

CONSTRUCTOR()
{
    // Set up a simple exit system call.
    __set_syscall(SYS_exit, sys_exit);
    __set_syscall(SYS_exit_group, sys_exit);
}
