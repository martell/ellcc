// RUN: llvm-mc -filetype=obj -triple=x86_64-unknown-linux %s -o %t.o
// RUN: ld.lld2 %t.o -o %t -shared
// RUN: llvm-readobj -s -r %t | FileCheck %s
// RUN: llvm-objdump -d %t | FileCheck --check-prefix=DISASM %s

bar:
	call foo@gotpcrel

        .hidden foo
        .global foo
foo:
        nop

// 0x2090 - 0x1000 - 5 = 4235
// DISASM:      bar:
// DISASM-NEXT:   1000: {{.*}} callq 4235

// DISASM:      foo:
// DISASM-NEXT:   1005: {{.*}} nop

// CHECK:      Name: .got
// CHECK-NEXT: Type: SHT_PROGBITS
// CHECK-NEXT: Flags [
// CHECK-NEXT:   SHF_ALLOC
// CHECK-NEXT:   SHF_WRITE
// CHECK-NEXT: ]
// CHECK-NEXT: Address: 0x2090
// CHECK-NEXT: Offset:
// CHECK-NEXT: Size: 8

// CHECK:      Relocations [
// CHECK-NEXT:   Section ({{.*}}) .rela.dyn {
// CHECK-NEXT:     0x2090 R_X86_64_RELATIVE - 0x1005
// CHECK-NEXT:   }
// CHECK-NEXT: ]
