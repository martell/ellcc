// RUN: llvm-mc -filetype=obj -triple=aarch64-unknown-linux %s -o %t.o
// RUN: ld.lld -shared %t.o -o %t.so
// RUN: llvm-readobj -s -r %t.so | FileCheck %s
// RUN: llvm-objdump -d %t.so | FileCheck --check-prefix=DISASM %s
// REQUIRES: aarch64

// CHECK:      Name: .got
// CHECK-NEXT: Type: SHT_PROGBITS
// CHECK-NEXT: Flags [
// CHECK-NEXT:   SHF_ALLOC
// CHECK-NEXT:   SHF_WRITE
// CHECK-NEXT: ]
// CHECK-NEXT: Address: 0x2090
// CHECK-NEXT: Offset:
// CHECK-NEXT: Size: 8
// CHECK-NEXT: Link: 0
// CHECK-NEXT: Info: 0
// CHECK-NEXT: AddressAlignment: 8

// CHECK:      Relocations [
// CHECK-NEXT:   Section ({{.*}}) .rela.dyn {
// CHECK-NEXT:     0x2090 R_AARCH64_GLOB_DAT dat 0x0
// CHECK-NEXT:   }
// CHECK-NEXT: ]

// Page(0x2098) - Page(0x1000) = 0x1000 = 4096
// 0x2098 & 0xff8 = 0x98 = 152

// DISASM: main:
// DISASM-NEXT:     1000: {{.*}} adrp x0, #4096
// DISASM-NEXT:     1004: {{.*}} ldr x0, [x0, #144]

.global main,foo,dat
.text
main:
    adrp x0, :got:dat
    ldr x0, [x0, :got_lo12:dat]
.data
dat:
    .word 42
