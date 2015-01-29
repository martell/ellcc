/* Handle the exit system call.
 */
#include <syscalls.h>           // For syscall numbers.
#include <kernel.h>
#include "crt1.h"

// Make simple exit a loadable feature.
FEATURE_CLASS(simple_exit, exit)

static int sys_exit(int status)
{
  for( ;; )
    continue;
}

strong_alias(sys_exit, sys_exit_group);

ELK_PRECONSTRUCTOR()
{
  // Set up a simple exit system call.
  SYSCALL(exit);
  SYSCALL(exit_group);
}
