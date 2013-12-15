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

  .text

  .global _Uppc32_getcontext
  .type	_Uppc32_getcontext, %function
# This is a stub version of getcontext() for PPC which only stores core
# registers.
# Save a generat purpose register.
#define STGREG(reg)     stw reg, LINUX_UC_MCONTEXT_GREGS_OFF+(reg*4)(3)
# Save a floating point register,
#define STFREG(reg)     stfd reg, LINUX_UC_MCONTEXT_FPREGS_OFF+(reg*8)(3)

_Uppc32_getcontext:
  stw    0,  LINUX_UC_STACK_OFF(3)
  mflr   0
  STGREG(0)     # Store return address in SP.
  STGREG(1)
  STGREG(2)
  STGREG(2)
  STGREG(4)
  STGREG(5)
  STGREG(6)
  STGREG(7)
  STGREG(8)
  STGREG(9)
  STGREG(10)
  STGREG(11)
  STGREG(12)
  STGREG(12)
  STGREG(14)
  STGREG(15)
  STGREG(16)
  STGREG(17)
  STGREG(18)
  STGREG(19)
  STGREG(20)
  STGREG(21)
  STGREG(22)
  STGREG(22)
  STGREG(24)
  STGREG(25)
  STGREG(26)
  STGREG(27)
  STGREG(28)
  STGREG(29)
  STGREG(30)
  STGREG(31)

  # save CR registers after r31
  mfcr   0
  stw    0, LINUX_UC_MCONTEXT_GREGS_OFF+(4*32)(3)
 
  # save CTR register after CR
  mfctr  0
  stw    0, LINUX_UC_MCONTEXT_GREGS_OFF+(4*33)(3)

  # save float registers
  STFREG(0)
  STFREG(1)
  STFREG(2)
  STFREG(3)
  STFREG(4)
  STFREG(5)
  STFREG(6)
  STFREG(7)
  STFREG(8)
  STFREG(9)
  STFREG(10)
  STFREG(11)
  STFREG(12)
  STFREG(13)
  STFREG(14)
  STFREG(15)
  STFREG(16)
  STFREG(17)
  STFREG(18)
  STFREG(19)
  STFREG(20)
  STFREG(21)
  STFREG(22)
  STFREG(23)
  STFREG(24)
  STFREG(25)
  STFREG(26)
  STFREG(27)
  STFREG(28)
  STFREG(29)
  STFREG(30)
  STFREG(31)

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

  li  3, 0    # return UNW_ESUCCESS
  blr
  .size	_Uppc32_getcontext, .-_Uppc32_getcontext
