// Compile and run for every target.
// RUN: %ecc -g -o %t %s ; %t | FileCheck -check-prefix=CHECK %s
// RUN: %armecc -g -o %t %s ; %armrun %t | FileCheck -check-prefix=CHECK %s
// RUN: %armebecc -g -o %t %s ; %armebrun %t | FileCheck -check-prefix=CHECK %s
// RUN: %i386ecc -g -o %t %s ; %i386run %t | FileCheck -check-prefix=CHECK %s
// RUN: %microblazeecc -g -o %t %s ; %microblazerun %t | FileCheck -check-prefix=CHECK %s
// RUN: %mipsecc -g -o %t %s ; %mipsrun %t | FileCheck -check-prefix=CHECK %s
// RUN: %mipselecc -g -o %t %s ; %mipselrun %t | FileCheck -check-prefix=CHECK %s
// RUN: %ppcecc -g -o %t %s ; %ppcrun %t | FileCheck -check-prefix=CHECK %s
// FAIL: %ppc64ecc -g -o %t %s ; %ppc64run %t | FileCheck -check-prefix=CHECK %s
// RUN: %x86_64ecc -g -o %t %s ; %x86_64run %t | FileCheck -check-prefix=CHECK %s
#include <stdlib.h>
#include <stdio.h>
int main()
{
    printf("Before exit\n");
    exit(0);
    printf("After exit\n");
}
// CHECK: Before exit
// CHECK-NOT: After exit
