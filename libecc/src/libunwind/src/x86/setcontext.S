/* libunwind - a platform-independent unwind library
   Copyright (C) 2007 Google, Inc
	Contributed by Arun Sharma <arun.sharma@google.com>
   Copyright (C) 2010 Konstantin Belousov <kib@freebsd.org>

This file is part of libunwind.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.  */

#include "ucontext_i.h"
#if defined __linux__
#ifdef __ELLCC__
#include <sys/syscall.h>
#else
#include <asm/unistd.h>
#endif
#define	SIG_SETMASK   2
#define	SIGSET_BYTE_SIZE   (64/8)
#elif defined __FreeBSD__
#include <sys/syscall.h>
#endif

/*  int _Ux86_setcontext (const ucontext_t *ucp)

  Restores the machine context provided.
  Unlike the libc implementation, doesn't clobber %eax
  
*/
// RICH: FIXME
	.global _Ux86_setcontext
	.type _Ux86_setcontext, @function

_Ux86_setcontext:

#if defined __linux__
	/* restore signal mask
           sigprocmask(SIG_SETMASK, ucp->uc_sigmask, NULL, sizeof(sigset_t)) */
#if RICH
	push %edi
	mov $__NR_rt_sigprocmask, %eax
	lea UC_SIGMASK(%edi), %esi
	mov $SIG_SETMASK, %edi
	xor %edx, %edx
	mov $SIGSET_BYTE_SIZE, %e10
	syscall
	pop %edi

        /* restore fp state */
	mov    UC_MCONTEXT_FPREGS_PTR(%edi),%e8
	fldenv (%e8)
	ldmxcsr FPREGS_OFFSET_MXCSR(%e8)
#endif
#elif defined __FreeBSD__
	/* restore signal mask */
	pushq	%edi
	xorl	%edx,%edx
	leaq	UC_SIGMASK(%edi),%esi
	movl	$3,%edi/* SIG_SETMASK */
	movl	$SYS_sigprocmask,%eax
	movq	%ecx,%e10
	syscall
	popq	%edi

	/* restore fp state */
	cmpq $UC_MCONTEXT_FPOWNED_FPU,UC_MCONTEXT_OWNEDFP(%edi)
	jne 1f
	cmpq $UC_MCONTEXT_FPFMT_XMM,UC_MCONTEXT_FPFORMAT(%edi)
	jne 1f
	fxrstor UC_MCONTEXT_FPSTATE(%edi)
1:
#else
#error Port me
#endif

	/* restore the rest of the state */
	mov    UC_MCONTEXT_GREGS_RBX(%edi),%ebx
	mov    UC_MCONTEXT_GREGS_RBP(%edi),%ebp
	mov    UC_MCONTEXT_GREGS_RSI(%edi),%esi
	mov    UC_MCONTEXT_GREGS_RDX(%edi),%edx
	mov    UC_MCONTEXT_GREGS_RAX(%edi),%eax
	mov    UC_MCONTEXT_GREGS_RCX(%edi),%ecx
	mov    UC_MCONTEXT_GREGS_RSP(%edi),%esp

        /* push the return address on the stack */
	mov    UC_MCONTEXT_GREGS_RIP(%edi),%ecx
	push   %ecx

	mov    UC_MCONTEXT_GREGS_RCX(%edi),%ecx
	mov    UC_MCONTEXT_GREGS_RDI(%edi),%edi
	ret

	.size _Ux86_setcontext, . - _Ux86_setcontext

      /* We do not need executable stack.  */
      .section        .note.GNU-stack,"",@progbits
