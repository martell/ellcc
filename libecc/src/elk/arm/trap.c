/*-
 * Copyright (c) 2005-2009, Kohsuke Ohtani
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * trap.c - called from the abort handler when a processor detect abort.
 */

#include <signal.h>

#include "config.h"
#include "kernel.h"
#include "thread.h"
#include "cpu.h"
#include "cpufunc.h"
#include "context.h"
#include "crt1.h"
#include "trap.h"

FEATURE(trap)

#define DEBUG

is_used static int dummy(void)
{
  return 0;
}

weak_alias(dummy, gettid);

#ifdef DEBUG
/*
 * Trap name
 */
static char *const trap_name[] = {
  "Undefined Instruction",
  "Prefetch Abort",
  "Data Abort"
};
#define MAXTRAP (sizeof(trap_name) / sizeof(void *) - 1)
#endif  /* !DEBUG */

/** Abort/exception mapping table.
 * ARM exception is translated to the architecture
 * independent exception code.
 */
#ifdef RICH     // Save for when used.
static const int exception_map[] = {
  SIGILL,                               // Undefined instruction.
  SIGSEGV,                              // Prefech abort.
  SIGSEGV,                              // Data abort.
};
#endif

void trap_dump(int (*print)(const char * __restrict, ...),
               const char *s, context_t *r)
{
  print("%s:\n", s);
  print("Context frame %x\n", r);
  print(" r0  %08x r1  %08x r2  %08x r3  %08x r4  %08x r5  %08x\n",
        r->r0, r->r1, r->r2, r->r3, r->r4, r->r5);
  print(" r6  %08x r7  %08x r8  %08x r9  %08x r10 %08x r11 %08x\n",
        r->r6, r->r7, r->r8, r->r9, r->r10, r->r11);
  print(" r12 %08x sp  %08x lr  %08x tls %08x pc  %08x cpsr %08x\n",
        r->r12, r->sp, r->lr, r->tls, r->pc, r->cpsr);

  print(" >> interrupt is %s\n",
        (r->cpsr & PSR_INT_MASK) ? "disabled" : "enabled");
}

/** Trap handler
 * Invoke the exception handler if it is needed.
 */
int trap_handler(u_long trap_no, context_t *regs)
{
  if ((regs->cpsr & PSR_MODE) == PSR_SVC_MODE &&
      trap_no == TRAP_DATA_ABORT &&
      (regs->pc - 4 == (char *)known_fault1 ||
       regs->pc - 4 == (char *)known_fault2 ||
       regs->pc - 4 == (char *)known_fault3)) {
    DPRINTF(THRDB_FAULT, ("\n*** Detect Fault! address=%x tid=%d ***\n",
                          get_faultaddress(), gettid()));
    regs->pc = (char *)copy_fault;
    return 1;
  }

#ifdef DEBUG
  diag_printf("=============================\n");
  diag_printf("Trap %x: %s\n", (u_int)trap_no, trap_name[trap_no]);
  if (trap_no == TRAP_DATA_ABORT)
    diag_printf(" Fault address=%x\n", get_faultaddress());
  else if (trap_no == TRAP_PREFETCH_ABORT)
    diag_printf(" Fault address=%x\n", regs->pc);
  diag_printf("=============================\n");

  trap_dump(diag_printf, "Context", regs);
  diag_printf(" >> tid=%d\n", gettid());
  for (;;) 
    continue;

#endif
  if ((regs->cpsr & PSR_MODE) != PSR_USR_MODE)
    panic("Kernel exception");

#if RICH        // Send the exception to the process.
  exception_mark(exception_map[trap_no]);
  exception_deliver();
#endif
  return 1;
}

