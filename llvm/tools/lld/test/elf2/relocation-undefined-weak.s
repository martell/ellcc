// RUN: llvm-mc -filetype=obj -triple=x86_64-pc-linux %s -o %t
// RUN: ld.lld2 %t -o %tout
// RUN: llvm-readobj -sections %tout | FileCheck %s
// RUN: llvm-objdump -d %tout | FileCheck %s --check-prefix DISASM
// REQUIRES: x86

// Check that undefined weak symbols are treated as having a VA of 0.

.global _start
_start:
  movl $1, sym1(%rip)

.weak sym1

// CHECK:      Name: .text
// CHECK-NEXT: Type: SHT_PROGBITS
// CHECK-NEXT: Flags [
// CHECK-NEXT:   SHF_ALLOC
// CHECK-NEXT:   SHF_EXECINSTR
// CHECK-NEXT: ]
// CHECK-NEXT: Address: 0x11000

// Unfortunately FileCheck can't do math, so we have to check for explicit
// values:
// R_86_64_PC32 = 0 + (-8 - (0x11000 + 2)) = -69642

// DISASM: movl    $1, -69642(%rip)
