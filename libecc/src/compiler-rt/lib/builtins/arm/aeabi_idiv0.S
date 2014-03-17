//===-- aeabi_idiv0.S - EABI divide by 0 handling -------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "../assembly.h"

// RICH: Dummy implementation.

        .syntax unified
        .align 2
DEFINE_COMPILERRT_FUNCTION(__aeabi_idiv0)
	mov	r0, #0
	JMP(lr)
END_COMPILERRT_FUNCTION(__aeabi_idiv0)
