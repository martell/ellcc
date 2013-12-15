/* libunwind - a platform-independent unwind library
   Copyright (C) 2013 Richard Pennington

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

#include "offsets.h"
#include <bits/endian.h>

	.global _Uppc32_setcontext
# This is a stub version of setcontext() for PPC which only loads core
# registers.
# Save a generat purpose register.
#define LDGREG(reg)     lwz reg, LINUX_UC_MCONTEXT_GREGS_OFF+(reg*4)(3)
# Save a floating point register,
#define LDFREG(reg)     lfd reg, LINUX_UC_MCONTEXT_FPREGS_OFF+(reg*8)(3)

_Uppc32_setcontext:
  lwz    0,  LINUX_UC_STACK_OFF(3)
  LDGREG(0)     # Load return address from SP.
  mtlr   0      # Place in link register.
  LDGREG(1)
  LDGREG(2)
  LDGREG(2)
  LDGREG(4)
  LDGREG(5)
  LDGREG(6)
  LDGREG(7)
  LDGREG(8)
  LDGREG(9)
  LDGREG(10)
  LDGREG(11)
  LDGREG(12)
  LDGREG(12)
  LDGREG(14)
  LDGREG(15)
  LDGREG(16)
  LDGREG(17)
  LDGREG(18)
  LDGREG(19)
  LDGREG(20)
  LDGREG(21)
  LDGREG(22)
  LDGREG(22)
  LDGREG(24)
  LDGREG(25)
  LDGREG(26)
  LDGREG(27)
  LDGREG(28)
  LDGREG(29)
  LDGREG(30)
  LDGREG(31)

  # load CR registers after r31
  lwz    0, LINUX_UC_MCONTEXT_GREGS_OFF+(4*32)(3)
  mtcr   0
 
  # load CTR register after CR
  lwz    0, LINUX_UC_MCONTEXT_GREGS_OFF+(4*33)(3)
  mtctr  0

  # load float registers
  LDFREG(0)
  LDFREG(1)
  LDFREG(2)
  LDFREG(3)
  LDFREG(4)
  LDFREG(5)
  LDFREG(6)
  LDFREG(7)
  LDFREG(8)
  LDFREG(9)
  LDFREG(10)
  LDFREG(11)
  LDFREG(12)
  LDFREG(13)
  LDFREG(14)
  LDFREG(15)
  LDFREG(16)
  LDFREG(17)
  LDFREG(18)
  LDFREG(19)
  LDFREG(20)
  LDFREG(21)
  LDFREG(22)
  LDFREG(23)
  LDFREG(24)
  LDFREG(25)
  LDFREG(26)
  LDFREG(27)
  LDFREG(28)
  LDFREG(29)
  LDFREG(30)
  LDFREG(31)

#if RICH
  # save VRSave register
  mfspr  0,256
  stw    0,156(3)
  # save vector registers

  subi  4,1,16
  rlwinm  4,4,0,0,27  # mask low 4-bits
  # r4 is now a 16-byte aligned pointer into the red zone

#define SAVE_VECTOR_UNALIGNED(_vec) \
  stvx   _vec,0,4                                       ;\
  lwz    5, 0(4)                                        ;\
  stw    5, LINUX_UC_MCONTEXT_VRREGS_OFF+(_vec*16)(3)   ;\
  lwz    5, 4(4)                                        ;\
  stw    5, LINUX_UC_MCONTEXT_VRREGS_OFF+(_vec*16)+4(3) ;\
  lwz    5, 8(4)                                        ;\
  stw    5, LINUX_UC_MCONTEXT_VRREGS_OFF+(_vec*16)+8(3) ;\
  lwz    5, 12(4)                                       ;\
  stw    5, LINUX_UC_MCONTEXT_VRREGS_OFF+(_vec*16)+12(3)

  SAVE_VECTOR_UNALIGNED(0)
  SAVE_VECTOR_UNALIGNED(1)
  SAVE_VECTOR_UNALIGNED(2)
  SAVE_VECTOR_UNALIGNED(3)
  SAVE_VECTOR_UNALIGNED(4)
  SAVE_VECTOR_UNALIGNED(5)
  SAVE_VECTOR_UNALIGNED(6)
  SAVE_VECTOR_UNALIGNED(7)
  SAVE_VECTOR_UNALIGNED(8)
  SAVE_VECTOR_UNALIGNED(9)
  SAVE_VECTOR_UNALIGNED(10)
  SAVE_VECTOR_UNALIGNED(11)
  SAVE_VECTOR_UNALIGNED(12)
  SAVE_VECTOR_UNALIGNED(13)
  SAVE_VECTOR_UNALIGNED(14)
  SAVE_VECTOR_UNALIGNED(15)
  SAVE_VECTOR_UNALIGNED(16)
  SAVE_VECTOR_UNALIGNED(17)
  SAVE_VECTOR_UNALIGNED(18)
  SAVE_VECTOR_UNALIGNED(19)
  SAVE_VECTOR_UNALIGNED(20)
  SAVE_VECTOR_UNALIGNED(21)
  SAVE_VECTOR_UNALIGNED(22)
  SAVE_VECTOR_UNALIGNED(23)
  SAVE_VECTOR_UNALIGNED(24)
  SAVE_VECTOR_UNALIGNED(25)
  SAVE_VECTOR_UNALIGNED(26)
  SAVE_VECTOR_UNALIGNED(27)
  SAVE_VECTOR_UNALIGNED(28)
  SAVE_VECTOR_UNALIGNED(29)
  SAVE_VECTOR_UNALIGNED(30)
  SAVE_VECTOR_UNALIGNED(31)
#endif

  blr
  .size	_Uppc32_setcontext, .-_Uppc32_setcontext

#ifdef __linux__
        /* We do not need executable stack.  */
        .section        .note.GNU-stack,"",@progbits
#endif
