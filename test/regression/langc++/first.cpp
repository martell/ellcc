// Compile and run for every target.
// RUN: %armexx -o %t %s && %armrun %t  | FileCheck -check-prefix=CHECK %s
// RUN: %armebexx -o %t %s && %armebrun %t | FileCheck -check-prefix=CHECK %s
// RUN: %i386exx -o %t %s && %i386run %t | FileCheck -check-prefix=CHECK %s
// FAIL: %microblazeexx -o %t %s && %microblazerun %t | FileCheck -check-prefix=CHECK %s
// RUN: %mipsexx -o %t %s && %mipsrun %t | FileCheck -check-prefix=CHECK %s
// RUN: %mipselexx -o %t %s && %mipselrun %t | FileCheck -check-prefix=CHECK %s
// RUN: %ppcexx -o %t %s && %ppcrun %t | FileCheck -check-prefix=CHECK %s
// FAIL: %ppc64exx -o %t %s && %ppc64run %t | FileCheck -check-prefix=CHECK %s
// RUN: %x86_64exx -o %t %s && %x86_64run %t | FileCheck -check-prefix=CHECK %s
// CHECK: foo.i = 10
// CHECK: bye
#include <cstdio>

class Foo {
    int i;
public:
    Foo(int i) : i(i) { }
    int get() { return i; }
    ~Foo() { printf("bye\n"); }
};

int main(int argc, char** argv)
{
    Foo foo(10);
    printf("foo.i = %d\n", foo.get());
}
