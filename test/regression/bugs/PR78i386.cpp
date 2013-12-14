// XFAIL: *
// RUN: %i386exx -o %t %s && %i386run %t  | FileCheck -check-prefix=CHECK %s
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
