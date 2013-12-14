// XFAIL: *
// RUN: %ppcexx -o %t %s && %ppcrun %t  | FileCheck -check-prefix=CHECK %s
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
