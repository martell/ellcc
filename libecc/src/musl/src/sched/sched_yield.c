#include <sched.h>
#include "syscall.h"
#include "libc.h"

int __sched_yield()
{
	return syscall(SYS_sched_yield);
}

weak_alias(__sched_yield, sched_yield);
