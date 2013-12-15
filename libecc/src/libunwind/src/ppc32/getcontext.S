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
_Uppc32_getcontext:
  stw    0,  8(3)
  mflr   0
  stw    0,  0(3)  # store lr as ssr0
  stw    1, 12(3)
  stw    2, 16(3)
  stw    3, 20(3)
  stw    4, 24(3)
  stw    5, 28(3)
  stw    6, 32(3)
  stw    7, 36(3)
  stw    8, 40(3)
  stw    9, 44(3)
  stw    10, 48(3)
  stw    11, 52(3)
  stw    12, 56(3)
  stw    13, 60(3)
  stw    14, 64(3)
  stw    15, 68(3)
  stw    16, 72(3)
  stw    17, 76(3)
  stw    18, 80(3)
  stw    19, 84(3)
  stw    20, 88(3)
  stw    21, 92(3)
  stw    22, 96(3)
  stw    23,100(3)
  stw    24,104(3)
  stw    25,108(3)
  stw    26,112(3)
  stw    27,116(3)
  stw    28,120(3)
  stw    29,124(3)
  stw    30,128(3)
  stw    31,132(3)

  # save CR registers
  mfcr   0
  stw    0,136(3)
 
  # save CTR register
  mfctr  0
  stw    0,148(3)

  # save float registers
  stfd   0, 160(3)
  stfd   1, 168(3)
  stfd   2, 176(3)
  stfd   3, 184(3)
  stfd   4, 192(3)
  stfd   5, 200(3)
  stfd   6, 208(3)
  stfd   7, 216(3)
  stfd   8, 224(3)
  stfd   9, 232(3)
  stfd   10,240(3)
  stfd   11,248(3)
  stfd   12,256(3)
  stfd   13,264(3)
  stfd   14,272(3)
  stfd   15,280(3)
  stfd   16,288(3)
  stfd   17,296(3)
  stfd   18,304(3)
  stfd   19,312(3)
  stfd   20,320(3)
  stfd   21,328(3)
  stfd   22,336(3)
  stfd   23,344(3)
  stfd   24,352(3)
  stfd   25,360(3)
  stfd   26,368(3)
  stfd   27,376(3)
  stfd   28,384(3)
  stfd   29,392(3)
  stfd   30,400(3)
  stfd   31,408(3)

#if RICH
  # save VRSave register
  mfspr  0,256
  stw    0,156(3)
  # save vector registers

  subi  4,1,16
  rlwinm  4,4,0,0,27  # mask low 4-bits
  # r4 is now a 16-byte aligned pointer into the red zone

#define SAVE_VECTOR_UNALIGNED(_vec, _offset) \
  stvx   _vec,0,4          ;\
  lwz    5, 0(4)          ;\
  stw    5, _offset(3)    ;\
  lwz    5, 4(4)          ;\
  stw    5, _offset+4(3)  ;\
  lwz    5, 8(4)          ;\
  stw    5, _offset+8(3)  ;\
  lwz    5, 12(4)         ;\
  stw    5, _offset+12(3)

  SAVE_VECTOR_UNALIGNED( 0, 424+0x000)
  SAVE_VECTOR_UNALIGNED( 1, 424+0x010)
  SAVE_VECTOR_UNALIGNED( 2, 424+0x020)
  SAVE_VECTOR_UNALIGNED( 3, 424+0x030)
  SAVE_VECTOR_UNALIGNED( 4, 424+0x040)
  SAVE_VECTOR_UNALIGNED( 5, 424+0x050)
  SAVE_VECTOR_UNALIGNED( 6, 424+0x060)
  SAVE_VECTOR_UNALIGNED( 7, 424+0x070)
  SAVE_VECTOR_UNALIGNED( 8, 424+0x080)
  SAVE_VECTOR_UNALIGNED( 9, 424+0x090)
  SAVE_VECTOR_UNALIGNED(10, 424+0x0A0)
  SAVE_VECTOR_UNALIGNED(11, 424+0x0B0)
  SAVE_VECTOR_UNALIGNED(12, 424+0x0C0)
  SAVE_VECTOR_UNALIGNED(13, 424+0x0D0)
  SAVE_VECTOR_UNALIGNED(14, 424+0x0E0)
  SAVE_VECTOR_UNALIGNED(15, 424+0x0F0)
  SAVE_VECTOR_UNALIGNED(16, 424+0x100)
  SAVE_VECTOR_UNALIGNED(17, 424+0x110)
  SAVE_VECTOR_UNALIGNED(18, 424+0x120)
  SAVE_VECTOR_UNALIGNED(19, 424+0x130)
  SAVE_VECTOR_UNALIGNED(20, 424+0x140)
  SAVE_VECTOR_UNALIGNED(21, 424+0x150)
  SAVE_VECTOR_UNALIGNED(22, 424+0x160)
  SAVE_VECTOR_UNALIGNED(23, 424+0x170)
  SAVE_VECTOR_UNALIGNED(24, 424+0x180)
  SAVE_VECTOR_UNALIGNED(25, 424+0x190)
  SAVE_VECTOR_UNALIGNED(26, 424+0x1A0)
  SAVE_VECTOR_UNALIGNED(27, 424+0x1B0)
  SAVE_VECTOR_UNALIGNED(28, 424+0x1C0)
  SAVE_VECTOR_UNALIGNED(29, 424+0x1D0)
  SAVE_VECTOR_UNALIGNED(30, 424+0x1E0)
  SAVE_VECTOR_UNALIGNED(31, 424+0x1F0)
#endif

  li  3, 0    # return UNW_ESUCCESS
  blr
  .size	_Uppc32_getcontext, .-_Uppc32_getcontext
