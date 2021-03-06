// RUN: llvm-mc -filetype=obj -triple=x86_64-unknown-linux %s -o %t.o
// RUN: llvm-mc -filetype=obj -triple=x86_64-unknown-linux %p/Inputs/shared.s -o %t2.o
// RUN: ld.lld2 -shared %t2.o -o %t2.so
// RUN: ld.lld2 -shared %t.o %t2.so -o %t
// RUN: ld.lld2 %t.o %t2.so -o %t3
// RUN: llvm-readobj -s -r %t | FileCheck %s
// RUN: llvm-objdump -d %t | FileCheck --check-prefix=DISASM %s
// RUN: llvm-readobj -s -r %t3 | FileCheck --check-prefix=CHECK2 %s
// RUN: llvm-objdump -d %t3 | FileCheck --check-prefix=DISASM2 %s

// REQUIRES: x86

// CHECK:      Name: .plt
// CHECK-NEXT: Type: SHT_PROGBITS
// CHECK-NEXT: Flags [
// CHECK-NEXT:   SHF_ALLOC
// CHECK-NEXT:   SHF_EXECINSTR
// CHECK-NEXT: ]
// CHECK-NEXT: Address: 0x1020
// CHECK-NEXT: Offset:
// CHECK-NEXT: Size: 64
// CHECK-NEXT: Link: 0
// CHECK-NEXT: Info: 0
// CHECK-NEXT: AddressAlignment: 16

// CHECK:      Relocations [
// CHECK-NEXT:   Section ({{.*}}) .rela.plt {
// CHECK-NEXT:     0x20C8 R_X86_64_JUMP_SLOT bar 0x0
// CHECK-NEXT:     0x20D0 R_X86_64_JUMP_SLOT zed 0x0
// CHECK-NEXT:     0x20D8 R_X86_64_JUMP_SLOT _start 0x0
// CHECK-NEXT:   }
// CHECK-NEXT: ]

// CHECK2:      Name: .plt
// CHECK2-NEXT: Type: SHT_PROGBITS
// CHECK2-NEXT: Flags [
// CHECK2-NEXT:   SHF_ALLOC
// CHECK2-NEXT:   SHF_EXECINSTR
// CHECK2-NEXT: ]
// CHECK2-NEXT: Address: 0x11020
// CHECK2-NEXT: Offset:
// CHECK2-NEXT: Size: 48
// CHECK2-NEXT: Link: 0
// CHECK2-NEXT: Info: 0
// CHECK2-NEXT: AddressAlignment: 16

// CHECK2:      Relocations [
// CHECK2-NEXT:   Section ({{.*}}) .rela.plt {
// CHECK2-NEXT:     0x120C8 R_X86_64_JUMP_SLOT bar 0x0
// CHECK2-NEXT:     0x120D0 R_X86_64_JUMP_SLOT zed 0x0
// CHECK2-NEXT:   }
// CHECK2-NEXT: ]

// Unfortunately FileCheck can't do math, so we have to check for explicit
// values:

// 0x1030 - (0x1000 + 5) = 43
// 0x1030 - (0x1005 + 5) = 38
// 0x1040 - (0x100a + 5) = 49
// 0x1048 - (0x100a + 5) = 60

// DISASM:      _start:
// DISASM-NEXT:   1000:  e9 {{.*}}       jmp  43
// DISASM-NEXT:   1005:  e9 {{.*}}       jmp  38
// DISASM-NEXT:   100a:  e9 {{.*}}       jmp  49
// DISASM-NEXT:   100f:  e9 {{.*}}       jmp  60

// 0x20C8 - 0x1036  = 4242
// 0x20D0 - 0x1046  = 4234
// 0x20D8 - 0x1056  = 4226

// DISASM:      Disassembly of section .plt:
// DISASM-NEXT: .plt:
// DISASM-NEXT:   1020:  ff 35 92 10 00 00  pushq 4242(%rip)
// DISASM-NEXT:   1026:  ff 25 94 10 00 00  jmpq *4244(%rip)
// DISASM-NEXT:   102c:  0f 1f 40 00        nopl (%rax)
// DISASM-NEXT:   1030:  ff 25 92 10 00 00  jmpq *4242(%rip)
// DISASM-NEXT:   1036:  68 00 00 00 00     pushq $0
// DISASM-NEXT:   103b:  e9 e0 ff ff ff     jmp -32 <bar+1020>
// DISASM-NEXT:   1040:  ff 25 8a 10 00 00  jmpq *4234(%rip)
// DISASM-NEXT:   1046:  68 01 00 00 00     pushq $1
// DISASM-NEXT:   104b:  e9 d0 ff ff ff     jmp -48 <bar+1020>
// DISASM-NEXT:   1050:  ff 25 82 10 00 00  jmpq *4226(%rip)
// DISASM-NEXT:   1056:  68 02 00 00 00     pushq $2
// DISASM-NEXT:   105b:  e9 c0 ff ff ff     jmp -64 <bar+1020>

// 0x11030 - (0x11000 + 1) - 4 = 43
// 0x11030 - (0x11005 + 1) - 4 = 38
// 0x11040 - (0x1100a + 1) - 4 = 49
// 0x11000 - (0x1100f + 1) - 4 = -20

// DISASM2:      _start:
// DISASM2-NEXT:   11000:  e9 {{.*}}     jmp  43
// DISASM2-NEXT:   11005:  e9 {{.*}}     jmp  38
// DISASM2-NEXT:   1100a:  e9 {{.*}}     jmp  49
// DISASM2-NEXT:   1100f:  e9 {{.*}}     jmp  -20

// 0x120C8 - 0x11036  = 4242
// 0x120D0 - 0x11046  = 4234

// DISASM2:      Disassembly of section .plt:
// DISASM2-NEXT: .plt:
// DISASM2-NEXT:  11020:  ff 35 92 10 00 00   pushq 4242(%rip)
// DISASM2-NEXT:  11026:  ff 25 94 10 00 00   jmpq *4244(%rip)
// DISASM2-NEXT:  1102c:  0f 1f 40 00         nopl  (%rax)
// DISASM2-NEXT:  11030:  ff 25 92 10 00 00   jmpq *4242(%rip)
// DISASM2-NEXT:  11036:  68 00 00 00 00      pushq $0
// DISASM2-NEXT:  1103b:  e9 e0 ff ff ff      jmp -32 <bar+11020>
// DISASM2-NEXT:  11040:  ff 25 8a 10 00 00   jmpq *4234(%rip)
// DISASM2-NEXT:  11046:  68 01 00 00 00      pushq $1
// DISASM2-NEXT:  1104b:  e9 d0 ff ff ff      jmp -48 <bar+11020>
// DISASM2-NEXT-NOT: 110C0

.global _start
_start:
  jmp bar@PLT
  jmp bar@PLT
  jmp zed@PLT
  jmp _start@plt
