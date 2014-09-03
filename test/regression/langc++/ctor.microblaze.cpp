// BUG 82
// XFAIL: *
// RUN: %microblazeexx -o %t %s && %microblazerun %t | FileCheck -check-prefix=CHECK %s
// CHECK: foo = 42
#include <stdio.h>

int f()
{
    return 42;
}

int foo = f();

int main()
{
    printf("foo = %d\n", foo);
}
