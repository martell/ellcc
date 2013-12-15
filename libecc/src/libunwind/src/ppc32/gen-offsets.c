/** Program to generate offsets.h
 * This program should be cross compiled for the ppc. It can then be executed with
 * qemu to generate the proper offsets.h file.
 * ~/ellcc/bin/ecc -target ppc-ellcc-linux -o gen-offsets gen-offsets.c
 * ~/ellcc/bin/qemu-ppc gen-offsets > offsets.h
 */
#include <stdio.h>
#include <stddef.h>
#include <ucontext.h>

#define UC(N,X) \
  printf ("#define LINUX_UC_" N "_OFF\t0x%zX\n", offsetof (ucontext_t, X))

#define SC(N,X) \
  printf ("#define LINUX_SC_" N "_OFF\t0x%zX\n", offsetof (struct sigcontext, X))

int
main (void)
{
  printf (
"/* Linux-specific definitions: */\n\n"

"/* Define various structure offsets to simplify cross-compilation.  */\n\n"

"/* Offsets for ppc Linux \"ucontext_t\":  */\n\n");

  UC ("FLAGS", uc_flags);
  UC ("LINK", uc_link);
  UC ("STACK", uc_stack);
  UC ("MCONTEXT", uc_mcontext);
  UC ("SIGMASK", uc_sigmask);

  UC ("MCONTEXT_GREGS", uc_mcontext.gregs);
  UC ("MCONTEXT_FPREGS", uc_mcontext.fpregs);
  UC ("MCONTEXT_VRREGS", uc_mcontext.vrregs);

  return 0;
}
