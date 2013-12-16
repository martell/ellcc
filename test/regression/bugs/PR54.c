// XFAIL: *
// FIXES: test in stdlib/inttypes.c.
// RUN: %microblazeecc -o %t %s -lm && %microblazerun %t
#include <stdio.h>

int main()
{
    signed long long x, y;            

    x = 100;      
    y = 0x8000000000000000ULL;
    return x > y;
}
