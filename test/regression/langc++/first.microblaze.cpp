// BUG 82
// XFAIL: *
// RUN: %microblazeexx -o %t %s && %microblazerun %t | FileCheck -check-prefix=CHECK %s
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
