// X FAIL: *
// RUN: %armexx -o %t %s && %armrun %t  | FileCheck -check-prefix=CHECK %s
// RUN: %armebexx -o %t %s && %armebrun %t | FileCheck -check-prefix=CHECK %s
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
