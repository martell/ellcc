// XFAIL: *
// RUN: %mipsexx -o %t %s && %mipsrun %t  | FileCheck -check-prefix=CHECK %s
// RUN: %mipselexx -o %t %s && %mipselrun %t | FileCheck -check-prefix=CHECK %s
// CHECK: foo!
#include <stdio.h>

int f()
{
    throw 1;
}
int main()
{
    int i;
    try {
        f();
    } catch (int i) {
        printf("foo!\n");
    }
}
